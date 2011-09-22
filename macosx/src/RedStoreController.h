#import <Cocoa/Cocoa.h>
#import "TaskWrapper.h"

@interface RedStoreController : NSObject <TaskWrapperController>
{
  IBOutlet NSTextField *urlTextField;
  IBOutlet NSTextView *logTextView;
  IBOutlet NSWindow *mainWindow;
  IBOutlet NSButton *startStopButton;

  TaskWrapper *redstoreTask;
  bool processRunning;
}


- (IBAction)startStop:(id)sender;
- (IBAction)saveLogAs:(id)sender;
- (void)updateUrlField;

@end
