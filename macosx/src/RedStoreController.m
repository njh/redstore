#import "RedStoreController.h"

@implementation RedStoreController


- (IBAction)startStop:(id)sender
{
    if (processRunning) {
        [redstoreTask stopProcess];
    } else {
		NSString *redstorePath = [[NSBundle mainBundle] pathForAuxiliaryExecutable:@"redstore-cli"];

        // If the task is still sitting around from the last run, release it
        if (redstoreTask!=nil) [redstoreTask release];

		NSMutableArray *args = [NSMutableArray arrayWithObjects:redstorePath,
						 @"-p", [httpPortTextField stringValue],
						 @"-b", [addressTextField stringValue],
						 nil];

		if ([verboseCheckbox intValue]) {
			[args addObject:@"-v"];
		}

		switch ([storageType indexOfSelectedItem]) {
		    case 0:
				[args addObject:@"-s"]; [args addObject:@"hashes"];
				[args addObject:@"-t"]; [args addObject:@"hash-type='memory'"];
				break;
			default:
				NSLog(@"Unknown storage type: %@", [storageType titleOfSelectedItem]);
				break;
		}

#ifdef DEBUG
		NSLog(@"Command line arguments: %@", args);
#endif

		// Create the new task and start it running
        redstoreTask=[[TaskWrapper alloc] initWithController:self arguments:args];
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
		NSString* url = [NSString stringWithFormat:@"http://%@:%@/", [addressTextField stringValue], [httpPortTextField stringValue]];
		NSMutableAttributedString* attrString = [[NSMutableAttributedString alloc] initWithString: url];
		NSRange range = NSMakeRange(0, [attrString length]);

		[attrString addAttribute:NSLinkAttributeName value:url range:range];

		[urlTextField setSelectable: YES];
		[urlTextField setAllowsEditingTextAttributes: YES];
		[urlTextField setAttributedStringValue: [attrString autorelease]];

		// FIXME: why is this needed to get the hyperlink to display?
		[urlTextField selectText: self];
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

- (void) applicationDidFinishLaunching: (NSNotification *)note
{
}

- (void) applicationWillTerminate: (NSNotification *)note
{
	[redstoreTask stopProcess];
}

@end
