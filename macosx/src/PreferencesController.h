#import <Cocoa/Cocoa.h>

enum RedstoreStorageTypes {
	inmemoryHashes = 0,
	berkeleyDb,
	sqlite
};

@interface PreferencesController : NSWindowController {

}
@end
