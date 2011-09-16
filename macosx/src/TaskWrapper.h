#import <Foundation/Foundation.h>

@protocol TaskWrapperController

- (void)appendAttributedString:(NSAttributedString *)attrString;
- (void)appendString:(NSString *)string;
- (void)processStarted;
- (void)processFinished;

@end

@interface TaskWrapper : NSObject {
  NSTask    *task;
  id				<TaskWrapperController>controller;
  NSArray   *arguments;
}

- (id)initWithController:(id <TaskWrapperController>)controller arguments:(NSArray *)args;
- (void) startProcess;
- (void) stopProcess;
- (void) getData: (NSNotification *)aNotification;
- (void) taskDidTerminate: (NSNotification *)aNotification;

@end
