#import <Cocoa/Cocoa.h>
#import "PreferencesController.h"

@implementation PreferencesController

static NSDictionary *defaultValues() {
    static NSDictionary *dict = nil;
    if (!dict) {
        dict = [[NSDictionary alloc] initWithObjectsAndKeys:
                @"8080", @"http.port",
                @"localhost", @"http.address",
                [NSNumber numberWithBool:YES], @"logging.verbose",
                [NSNumber numberWithInt:0], @"storage.type",
				@"redstore", @"storage.name",
				NSHomeDirectory(), @"storage.path",
				nil];
    }
    return dict;
}

+ (void)initialize {
    [[NSUserDefaults standardUserDefaults] registerDefaults:defaultValues()];
    [[NSUserDefaultsController sharedUserDefaultsController] setInitialValues:defaultValues()];
}

- (id)init {
    return [super initWithWindowNibName:@"PreferencesWindow"];
}

- (void)windowDidLoad {
    NSWindow *window = [self window];
    [window setHidesOnDeactivate:NO];
    [window setExcludedFromWindowsMenu:YES];
}

/* We do this to catch the case where the user enters a value into one of the text fields
   but closes the window without hitting enter or tab.
 */
- (BOOL)windowShouldClose:(NSWindow *)window {
    return [window makeFirstResponder:nil]; // validate editing
}

- (void)windowWillClose:(NSNotification *)notification {
	// Force sync of preferences
	[[NSUserDefaults standardUserDefaults] synchronize];
}

@end
