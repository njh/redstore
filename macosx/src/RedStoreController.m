#import "RedStoreController.h"
#import "PreferencesController.h"

@implementation RedStoreController


- (IBAction)startStop:(id)sender
{
    if (processRunning) {
        [redstoreTask stopProcess];
    } else {
		NSString *redstorePath = [[NSBundle mainBundle] pathForAuxiliaryExecutable:@"redstore-cli"];
		NSMutableArray *args = nil;

        // If the task is still sitting around from the last run, release it
        if (redstoreTask!=nil) {
			[redstoreTask release];
			redstoreTask = nil;
		}

		// Check that the working directory exists
		BOOL isDir = NO;
		NSString *cwd = [[NSUserDefaults standardUserDefaults] stringForKey:@"storage.path"];
		if (![[NSFileManager defaultManager] fileExistsAtPath:cwd isDirectory:&isDir] || !isDir) {
			NSBeginAlertSheet(@"Directory not found", @"Ok", nil, nil, mainWindow,
							  nil, nil, nil, nil, @"Storage directory does not exist.");
			return;
		}

		// Build up the connect-line arguments
		args = [NSMutableArray arrayWithObjects:
				@"-p", [[NSUserDefaults standardUserDefaults] stringForKey:@"http.port"],
				@"-b", [[NSUserDefaults standardUserDefaults] stringForKey:@"http.address"],
			    nil];

		if ([[NSUserDefaults standardUserDefaults] boolForKey:@"logging.verbose"]) {
			[args addObject:@"-v"];
		}

		if ([[NSUserDefaults standardUserDefaults] boolForKey:@"storage.new"]) {
			[args addObject:@"-n"];
		}

		int type = [[NSUserDefaults standardUserDefaults] integerForKey:@"storage.type"];
		switch (type) {
		    case inmemoryHashes:
				[args addObject:@"-s"]; [args addObject:@"hashes"];
				[args addObject:@"-t"]; [args addObject:@"hash-type='memory'"];
				break;
			case berkeleyDb:
				[args addObject:@"-s"]; [args addObject:@"hashes"];
				[args addObject:@"-t"]; [args addObject:@"hash-type='bdb',dir='.'"];
				break;
			case sqlite:
				[args addObject:@"-s"]; [args addObject:@"sqlite"];
				break;
			default:
				NSLog(@"Unknown storage type: %d", type);
				break;
		}

		// Append the storage name
		[args addObject:[[NSUserDefaults standardUserDefaults] stringForKey:@"storage.name"]];

#ifdef DEBUG
		NSLog(@"Working directory: %@", cwd);
		NSLog(@"Command line arguments: %@", args);
#endif

		// Create the new task and start it running
        redstoreTask=[[TaskWrapper alloc] initWithController:self command:redstorePath];
		[redstoreTask setArguments: args];
		[redstoreTask setCurrentDirectoryPath: cwd];
        [redstoreTask startProcess];
    }
}

- (IBAction)saveLogAs:(id)sender
{
	NSSavePanel* panel = [NSSavePanel savePanel];
	[panel setTitle: @"Save Log As"];
	[panel setAllowedFileTypes:[NSArray arrayWithObject:@"log"]];

	if ([panel runModal] == NSFileHandlingPanelOKButton) {
		NSData *logData = [[[logTextView textStorage] string] dataUsingEncoding:NSUnicodeStringEncoding];
		BOOL success = [[NSFileManager defaultManager] createFileAtPath:[[panel URL] path] contents:logData attributes:nil];
		if (!success) {
			NSLog(@"Failed to open file");
		}
	}
}

- (void)updateUrlField
{
    if (processRunning)
    {
		NSString *port = [[NSUserDefaults standardUserDefaults] stringForKey:@"http.port"];
		NSString *address = [[NSUserDefaults standardUserDefaults] stringForKey:@"http.address"];
		NSString* url = [NSString stringWithFormat:@"http://%@:%@/", address, port];

		NSMutableAttributedString* attrString = [[NSMutableAttributedString alloc] initWithString: url];
		NSRange range = NSMakeRange(0, [attrString length]);

		[attrString addAttribute:NSLinkAttributeName value:url range:range];

		[urlTextField setSelectable: YES];
		[urlTextField setAllowsEditingTextAttributes: YES];
		[urlTextField setAttributedStringValue: [attrString autorelease]];

		// FIXME: why is this needed to get the hyperlink to display?
		[urlTextField selectText:nil];
		[[urlTextField currentEditor] setSelectedRange:NSMakeRange([attrString length], 0)];
	} else {
		[urlTextField setStringValue:@""];
	}
}

- (void)appendAttributedString:(NSAttributedString *)attrString
{
    [[logTextView textStorage] appendAttributedString: [attrString autorelease]];

    [self performSelector:@selector(scrollToVisible:) withObject:nil afterDelay:0.0];
}

- (void)appendString:(NSString *)string
{
	[self appendAttributedString: [[NSAttributedString alloc] initWithString:string]];
}

- (void)scrollToVisible:(id)sender {
    [logTextView scrollRangeToVisible:NSMakeRange([[logTextView string] length], 0)];
}

- (void)processStarted
{
    processRunning=YES;
    [logTextView setString:@""];
    [startStopButton setTitle:@"Stop"];
	[self updateUrlField];
}

- (void)processFinished
{
    processRunning=NO;
    [startStopButton setTitle:@"Start"];
	[self updateUrlField];

	// Clean up the memory used by the task
	if (redstoreTask) {
		[redstoreTask release];
		redstoreTask=nil;
	}
}

-(BOOL)windowShouldClose:(id)sender
{
    [NSApp terminate:nil];
    return YES;
}

-(void)awakeFromNib
{
    processRunning=NO;
    redstoreTask=nil;
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
	if ([[NSUserDefaults standardUserDefaults] integerForKey:@"autostart"]) {
		[self startStop:self];
	}
}

- (void) applicationWillTerminate: (NSNotification *)note
{
	[redstoreTask stopProcess];

	// Force sync of preferences to disk
	[[NSUserDefaults standardUserDefaults] synchronize];
}

@end
