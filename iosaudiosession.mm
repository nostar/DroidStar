#import <AVFoundation/AVFoundation.h>
#include <QtGlobal>

void configureiOSAudioSession() {
#if defined(Q_OS_IOS)
    AVAudioSession *session = [AVAudioSession sharedInstance];
    NSError *error = nil;

    BOOL success = [session setCategory:AVAudioSessionCategoryPlayAndRecord
                            withOptions:AVAudioSessionCategoryOptionDefaultToSpeaker
                                  error:&error];
    if (!success) {
        NSLog(@"Failed to set AVAudioSession category: %@", [error localizedDescription]);
        return;
    }

    success = [session setActive:YES error:&error];
    if (!success) {
        NSLog(@"Failed to activate AVAudioSession: %@", [error localizedDescription]);
        return;
    }
    NSLog(@"AVAudioSession successfully configured for simultaneous Speaker + Mic.");
#endif
}
