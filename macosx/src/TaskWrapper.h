#import <Foundation/Foundation.h>

@protocol TaskWrapperController

- (void)appendAttributedString:(NSAttributedString *)attrString;
- (void)appendString:(NSString *)string;
- (void)processStarted;
- (void)processFinished;

@end

@interface TaskWrapper : NSObject {
  NSTask    *task;
  id		<TaskWrapperController>controller;
  NSString  *commandPath;
  NSArray   *arguments;
  NSString  *directoryPath;
}

- (id)initWithController:(id <TaskWrapperController>)controller command:(NSString *)cmd;
- (void) startProcess;
- (void) stopProcess;
- (void) getData: (NSNotification *)aNotification;
- (void) taskDidTerminate: (NSNotification *)aNotification;
- (NSString *)currentDirectoryPath;
- (void) setCurrentDirectoryPath: (NSString *)path;
- (NSArray *)arguments;
- (void) setArguments: (NSArray *)path;
@end
