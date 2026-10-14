#include <AppKit/AppKit.h>
#include <QWidget>
#import <Cocoa/Cocoa.h>
#include <functional>
#import <objc/runtime.h>

// macOS delivers the mouse back/forward buttons (and trackpad page swipes) as swipe events, not as
// button presses, so Qt never reports Qt::BackButton / Qt::ForwardButton for them. Each press sends a
// swipe with deltaX 1 (back) or -1 (forward), preceded by one with deltaX 0. `navigate` receives
// whether to go forward and returns whether it handled the event.
void install_swipe_navigation_handler(std::function<bool(bool)> navigate) {
    static id monitor = nil;
    if (monitor != nil) return;
    monitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskSwipe handler:^NSEvent*(NSEvent* event) {
        if ([event deltaX] == 0) return event;
        return navigate([event deltaX] < 0) ? nil : event;
    }];
}

// Lets the tab bar take the title bar's place, like Firefox: the window content extends under a
// transparent title bar without a title, and the window buttons are moved down to the vertical center of
// the tab bar. AppKit lays the title bar out again when the window is resized, leaves full screen, or
// changes title bar settings, sometimes in a later layout pass without a window notification. This object
// therefore re-applies the layout after window resizes and whenever the title bar's frame changes. It is
// attached to (and owned by) its window. This file is compiled without ARC.
@interface YaviewTitlebarTabs : NSObject
@property (assign) NSWindow* window;
@property CGFloat barHeight;
@property BOOL enabled;
@property CGFloat originalContainerHeight;
@property (retain) NSArray<NSValue*>* originalButtonOrigins;
@property (retain) NSMutableArray* observers;
@property (assign) NSView* observedContainer;
@property (retain) id containerObserver;
@property BOOL applying;
- (CGFloat)apply;
- (void)restore;
@end

// Recent macOS versions blur the content under the title bar (NSTitlebarBackgroundView holds a
// "scroll pocket" blur, and NSTitlebarView a backdrop layer). With the tab bar as the title bar, that blur
// covered the tabs, so these views are hidden while tabs are in the title bar.
static void set_titlebar_blur_hidden(NSView* container, BOOL hidden) {
    for (NSView* child in container.subviews) {
        NSString* name = NSStringFromClass([child class]);
        if ([name isEqualToString:@"NSTitlebarBackgroundView"]) {
            child.hidden = hidden;
        }
        else if ([name isEqualToString:@"NSTitlebarView"]) {
            for (NSView* grandchild in child.subviews) {
                if ([grandchild.layer isKindOfClass:NSClassFromString(@"CABackdropLayer")]) {
                    grandchild.hidden = hidden;
                }
            }
        }
    }
}

@implementation YaviewTitlebarTabs
- (NSArray<NSButton*>*)buttons {
    NSWindow* window = self.window;
    NSButton* close = [window standardWindowButton:NSWindowCloseButton];
    NSButton* miniaturize = [window standardWindowButton:NSWindowMiniaturizeButton];
    NSButton* zoom = [window standardWindowButton:NSWindowZoomButton];
    if (close == nil || miniaturize == nil || zoom == nil) return nil;
    return @[close, miniaturize, zoom];
}

// Returns where the buttons end horizontally, in points from the window's left edge (0 when not applied).
- (CGFloat)apply {
    NSWindow* window = self.window;
    NSArray<NSButton*>* buttons = [self buttons];
    if (window == nil || buttons == nil || !self.enabled) return 0;
    // in full screen the buttons live in the menu bar that slides in from the top
    if ((window.styleMask & NSWindowStyleMaskFullScreen) != 0) return 0;

    NSView* container = buttons[0].superview.superview;
    if (self.applying) return 0;
    self.applying = YES;
    [self observeContainer:container];
    if (self.originalButtonOrigins == nil) {
        self.originalContainerHeight = container.frame.size.height;
        NSMutableArray* origins = [NSMutableArray array];
        for (NSButton* button in buttons) [origins addObject:[NSValue valueWithPoint:button.frame.origin]];
        self.originalButtonOrigins = origins;
    }
    NSRect container_frame = container.frame;
    container_frame.size.height = self.barHeight;
    container_frame.origin.y = window.frame.size.height - self.barHeight;
    container.frame = container_frame;
    set_titlebar_blur_hidden(container, YES);

    CGFloat right = 0;
    for (NSButton* button in buttons) {
        NSPoint origin = button.frame.origin;
        origin.y = round((self.barHeight - button.frame.size.height) / 2);
        [button setFrameOrigin:origin];
        right = MAX(right, NSMaxX(button.frame));
    }
    self.applying = NO;
    return right;
}

- (void)observeContainer:(NSView*)container {
    if (container == self.observedContainer) return;
    if (self.containerObserver != nil) {
        [[NSNotificationCenter defaultCenter] removeObserver:self.containerObserver];
        self.containerObserver = nil;
    }
    self.observedContainer = container;
    container.postsFrameChangedNotifications = YES;
    __block YaviewTitlebarTabs* observed_tabs = self; // not retained: this object owns the observer
    self.containerObserver = [[NSNotificationCenter defaultCenter] addObserverForName:NSViewFrameDidChangeNotification
        object:container queue:nil usingBlock:^(NSNotification*) { [observed_tabs apply]; }];
}

- (void)restore {
    NSWindow* window = self.window;
    NSArray<NSButton*>* buttons = [self buttons];
    if (window == nil || buttons == nil || self.originalButtonOrigins == nil) return;
    NSView* container = buttons[0].superview.superview;
    set_titlebar_blur_hidden(container, NO);
    NSRect container_frame = container.frame;
    container_frame.size.height = self.originalContainerHeight;
    container_frame.origin.y = window.frame.size.height - self.originalContainerHeight;
    container.frame = container_frame;
    for (NSUInteger i = 0; i < buttons.count; i++) {
        [buttons[i] setFrameOrigin:[self.originalButtonOrigins[i] pointValue]];
    }
}

- (void)dealloc {
    for (id observer in self.observers) [[NSNotificationCenter defaultCenter] removeObserver:observer];
    if (_containerObserver != nil) [[NSNotificationCenter defaultCenter] removeObserver:_containerObserver];
    [_containerObserver release];
    [_observers release];
    [_originalButtonOrigins release];
    [super dealloc];
}
@end


static char titlebar_tabs_key;

// Enables or disables the tabs-in-title-bar layout for the window of `winId`, for a tab bar `bar_height`
// points tall. Returns where the window buttons end, so the tabs can start after them (0 when disabled
// or in full screen).
extern "C" double setTabsInTitlebar(WId winId, bool enabled, double bar_height) {
    if (winId == 0) return 0;
    NSWindow* window = [(NSView*)winId window];
    if (window == nil) return 0;

    YaviewTitlebarTabs* tabs = objc_getAssociatedObject(window, &titlebar_tabs_key);
    if (tabs == nil) {
        if (!enabled) return 0;
        tabs = [[YaviewTitlebarTabs alloc] init];
        tabs.window = window;
        tabs.observers = [NSMutableArray array];
        // __block: the observer blocks must not retain `tabs`, which owns them
        __block YaviewTitlebarTabs* observed_tabs = tabs;
        for (NSNotificationName name in @[NSWindowDidResizeNotification, NSWindowDidEndLiveResizeNotification,
                                          NSWindowDidExitFullScreenNotification]) {
            [tabs.observers addObject:[[NSNotificationCenter defaultCenter] addObserverForName:name object:window
                queue:[NSOperationQueue mainQueue] usingBlock:^(NSNotification*) { [observed_tabs apply]; }]];
        }
        objc_setAssociatedObject(window, &titlebar_tabs_key, tabs, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        [tabs release]; // the window keeps it
    }

    if (!enabled) {
        if (tabs.enabled) {
            tabs.enabled = NO;
            [tabs restore];
            window.styleMask &= ~NSWindowStyleMaskFullSizeContentView;
            window.titlebarAppearsTransparent = NO;
            window.titleVisibility = NSWindowTitleVisible;
        }
        return 0;
    }

    tabs.barHeight = bar_height;
    if (!tabs.enabled) {
        tabs.enabled = YES;
        window.styleMask |= NSWindowStyleMaskFullSizeContentView;
        window.titlebarAppearsTransparent = YES;
        window.titleVisibility = NSWindowTitleHidden;
        // once more after AppKit's layout pass for the changes above
        __block YaviewTitlebarTabs* enabled_tabs = tabs;
        dispatch_async(dispatch_get_main_queue(), ^{ [enabled_tabs apply]; });
    }
    return [tabs apply];
}

// What a double click on the title bar does, following the system setting.
extern "C" void performTitlebarDoubleClick(WId winId) {
    if (winId == 0) return;
    NSWindow* window = [(NSView*)winId window];
    NSString* action = [[NSUserDefaults standardUserDefaults] stringForKey:@"AppleActionOnDoubleClick"];
    if ([action isEqualToString:@"Minimize"]) {
        [window performMiniaturize:nil];
    }
    else if (![action isEqualToString:@"None"]) {
        [window performZoom:nil];
    }
}

// Moves the window with the mouse, for a mouse press on an empty part of the tab bar.
extern "C" void startWindowDrag(WId winId) {
    if (winId == 0) return;
    NSWindow* window = [(NSView*)winId window];
    NSEvent* event = [NSApp currentEvent];
    if (window != nil && event != nil && event.type == NSEventTypeLeftMouseDown) {
        [window performWindowDragWithEvent:event];
    }
}

extern "C" void showLookupForString(WId winId, const char* text, double x, double y) {
    if (winId == 0 || text == nullptr) return;

    NSView* view = (NSView*)winId;
    NSString* nsText = [NSString stringWithUTF8String:text];

    NSPoint point = NSMakePoint(x, y);

    [view showDefinitionForAttributedString:[[NSAttributedString alloc] initWithString:nsText]
                                    atPoint:point];
}

extern "C" void changeTitlebarColor(WId winId, double red, double green, double blue, double alpha){
    if (winId == 0) return;
    NSView* view = (NSView*)winId;
    NSWindow* window = [view window];
    window.titlebarAppearsTransparent = YES;
    window.backgroundColor = [NSColor colorWithRed:red green:green blue:blue alpha: alpha];
}

@interface DraggableTitleView : NSView
@end

@implementation DraggableTitleView

// Handle mouse click events
- (void)mouseDown:(NSEvent *)event {
    // double-click to zoom
    if ([event clickCount] == 2) {
        [self.window zoom:nil];
    } else {
        // drag Window
        [self.window performWindowDragWithEvent:event];
    }
}

- (void)updateTrackingAreas {
    [self initTrackingArea];
}

-(void) initTrackingArea {
    NSTrackingAreaOptions options = (NSTrackingActiveAlways | NSTrackingInVisibleRect |
            NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved);

    NSTrackingArea *area = [[NSTrackingArea alloc] initWithRect:[self bounds]
        options:options
        owner:self
        userInfo:nil];

    [self addTrackingArea:area];
}
-(void)mouseEntered:(NSEvent *)event {
    if((self.window.styleMask & NSWindowStyleMaskFullScreen) == 0) {
        [[self.window standardWindowButton: NSWindowCloseButton] setHidden:NO];
        [[self.window standardWindowButton: NSWindowMiniaturizeButton] setHidden:NO];
        [[self.window standardWindowButton: NSWindowZoomButton] setHidden:NO];
    }
}

-(void)mouseExited:(NSEvent *)event {
    if((self.window.styleMask & NSWindowStyleMaskFullScreen) == 0) {
        [[self.window standardWindowButton: NSWindowCloseButton] setHidden:YES];
        [[self.window standardWindowButton: NSWindowMiniaturizeButton] setHidden:YES];
        [[self.window standardWindowButton: NSWindowZoomButton] setHidden:YES];
    }
}

@end

extern "C" void hideWindowTitleBar(WId winId) {
    if (winId == 0) return;

    NSView* nativeView = reinterpret_cast<NSView*>(winId);
    NSWindow* nativeWindow = [nativeView window];

    if(nativeWindow.titleVisibility == NSWindowTitleHidden){
        return;
    }

    [[nativeWindow standardWindowButton: NSWindowCloseButton] setHidden:YES];
    [[nativeWindow standardWindowButton: NSWindowMiniaturizeButton] setHidden:YES];
    [[nativeWindow standardWindowButton: NSWindowZoomButton] setHidden:YES];
    NSRect contentViewBounds = nativeWindow.contentView.bounds;

    DraggableTitleView *titleBarView = [[DraggableTitleView alloc] initWithFrame:NSMakeRect(0, 0, contentViewBounds.size.width, 22)];
    titleBarView.autoresizingMask = NSViewWidthSizable;
    titleBarView.wantsLayer = YES;
    titleBarView.layer.backgroundColor = [[NSColor clearColor] CGColor];

    [nativeWindow.contentView addSubview:titleBarView];

    [nativeWindow setTitleVisibility:NSWindowTitleHidden];
    [nativeWindow setStyleMask:[nativeWindow styleMask] | NSWindowStyleMaskFullSizeContentView];
    [nativeWindow setTitlebarAppearsTransparent:YES];
}
