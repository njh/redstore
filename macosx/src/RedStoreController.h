#import <Cocoa/Cocoa.h>
#import "TaskWrapper.h"

@interface RedStoreController : NSObject <TaskWrapperController>
{
  IBOutlet NSTextField *urlTextField;
  IBOutlet NSTextView *logTextView;
  IBOutlet NSWindow *mainWindow;
  IBOutlet NSButton *startStopButton;
  
  // Preferences
  IBOutlet NSTextField *httpPortTextField;
  IBOutlet NSTextField *addressTextField;
  IBOutlet NSButton *verboseCheckbox;
  IBOutlet NSPopUpButton *storageType;
  
  TaskWrapper *redstoreTask;
  bool processRunning;
}

- (IBAction)startStop:(id)sender;
- (IBAction)saveLogAs:(id)sender;
- (void)updateUrlField;

enum RedstoreStorageTypes {
	inmemoryHashes = 1
};

@end
