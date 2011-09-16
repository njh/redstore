#import "TaskWrapper.h"

@implementation TaskWrapper

- (id)initWithController:(id <TaskWrapperController>)cont arguments:(NSArray *)args
{
    self = [super init];
    controller = cont;
    arguments = [args retain];

    return self;
}

- (void)dealloc
{
	// Terminate the process
    [self stopProcess];

	// Remove ourselves from the notification center
    [[NSNotificationCenter defaultCenter] removeObserver:self name:NSFileHandleReadCompletionNotification object: [[task standardOutput] fileHandleForReading]];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:NSTaskDidTerminateNotification object:nil];

    [arguments release];
    [task release];
    [super dealloc];
}

- (void) startProcess
{
    // We first let the controller know that we are starting
    [controller processStarted];

	// Create and configure the process
    task = [[NSTask alloc] init];
    [task setStandardOutput: [NSPipe pipe]];
    [task setStandardError: [task standardOutput]];
    [task setLaunchPath: [arguments objectAtIndex:0]];
    [task setArguments: [arguments subarrayWithRange: NSMakeRange (1, ([arguments count] - 1))]];


	// Register for read notifications from the sub-process
    [[NSNotificationCenter defaultCenter] addObserver: self
											 selector: @selector(getData:)
												 name: NSFileHandleReadCompletionNotification
											   object: [[task standardOutput] fileHandleForReading]];

	// Notify if the process terminates
	[[NSNotificationCenter defaultCenter] addObserver: self
											 selector: @selector(taskDidTerminate:)
												 name: NSTaskDidTerminateNotification
											   object: nil];

	// Setup reading from the process in the background
    [[[task standardOutput] fileHandleForReading] readInBackgroundAndNotify];

    // launch the task asynchronously
    [task launch];
}

- (void) stopProcess
{
    [task terminate];
}

- (void) getData:(NSNotification *)aNotification
{
    NSData *data = [[aNotification userInfo] objectForKey:NSFileHandleNotificationDataItem];
    if ([data length])
    {
        // Send the log messages back to the application controller
        [controller appendString:[[[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding] autorelease]];
    } else {
        // End of File
        [self stopProcess];
    }

    // we need to schedule the file handle go read more data in the background again
    [[aNotification object] readInBackgroundAndNotify];
}

- (void) taskDidTerminate: (NSNotification *)aNotification
{
	// FIXME: make this string bold
	if ([task terminationStatus]) {
		[controller appendString:[NSString stringWithFormat:@"RedStore exited with status value: %d", [task terminationStatus]]];
	} else {
		[controller appendString:[NSString stringWithFormat:@"RedStore exited cleanly."]];
	}

	// Update the controller UI
	[controller processFinished];
}

@end

