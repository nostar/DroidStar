#import <AVFoundation/AVFoundation.h>
#include <QtGlobal>
#import <UIKit/UIKit.h>
#import <MediaPlayer/MediaPlayer.h>

static std::atomic<bool> g_appInBackground{false};

void setIOSAudioSessionCategory() {
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
    NSLog(@"AVAudioSession category: AVAudioSessionCategoryPlayAndRecord");
}

static inline void runOnMainThreadAsync(dispatch_block_t block) {
    if ([NSThread isMainThread]) {
        block();
    } else {
        dispatch_async(dispatch_get_main_queue(), block);
    }
}

@interface IOSAudioSession : NSObject
{
	UIBackgroundTaskIdentifier _bgTask;
	BOOL _isConnected;
	AVAudioPlayer *_silentPlayer;
}

+ (instancetype)sharedInstance;

@end

@implementation IOSAudioSession

+ (instancetype)sharedInstance {
    static IOSAudioSession *shared = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        shared = [[IOSAudioSession alloc] init];
    });
    return shared;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _bgTask = UIBackgroundTaskInvalid;
        _isConnected = NO;

        if ([NSThread isMainThread]) {
            g_appInBackground.store([UIApplication sharedApplication].applicationState == UIApplicationStateBackground,
                                    std::memory_order_relaxed);
        }

        [self setupNotificationObservers];
        [self createSilentAudioFile];
    }
    return self;
}

- (void)setupNotificationObservers {
    NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
    AVAudioSession *session = [AVAudioSession sharedInstance];

    [nc addObserver:self
           selector:@selector(handleAppDidEnterBackground:)
               name:UIApplicationDidEnterBackgroundNotification
             object:nil];

    [nc addObserver:self
           selector:@selector(handleAppWillEnterForeground:)
               name:UIApplicationWillEnterForegroundNotification
             object:nil];

    [nc addObserver:self
           selector:@selector(handleAppDidBecomeActive:)
               name:UIApplicationDidBecomeActiveNotification
             object:nil];

    [nc addObserver:self
           selector:@selector(handleAppWillResignActive:)
               name:UIApplicationWillResignActiveNotification
             object:nil];
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [self stopKeepAliveAudio];
    [self stopBackgroundTask];
}

- (void)createSilentAudioFile {
    NSString *tempDir = NSTemporaryDirectory();
    NSString *silentPath = [tempDir stringByAppendingPathComponent:@"silence_48k.wav"];

    if ([[NSFileManager defaultManager] fileExistsAtPath:silentPath]) {
        return;
    }

    int sampleRate = 48000;
    int numSamples = sampleRate; // 1 second at native 48 kHz so AQME never down-switches mixer rate
    int bitsPerSample = 16;
    int numChannels = 1;
    int byteRate = sampleRate * numChannels * bitsPerSample / 8;
    int blockAlign = numChannels * bitsPerSample / 8;
    int dataSize = numSamples * blockAlign;
    int fileSize = 36 + dataSize;

    NSMutableData *wavData = [NSMutableData data];

    // RIFF header
    [wavData appendBytes:"RIFF" length:4];
    uint32_t fileSizeLE = CFSwapInt32HostToLittle(fileSize);
    [wavData appendBytes:&fileSizeLE length:4];
    [wavData appendBytes:"WAVE" length:4];

    // fmt chunk
    [wavData appendBytes:"fmt " length:4];
    uint32_t fmtSize = CFSwapInt32HostToLittle(16);
    [wavData appendBytes:&fmtSize length:4];
    uint16_t audioFormat = CFSwapInt16HostToLittle(1); // PCM
    [wavData appendBytes:&audioFormat length:2];
    uint16_t channels = CFSwapInt16HostToLittle(numChannels);
    [wavData appendBytes:&channels length:2];
    uint32_t sampleRateLE = CFSwapInt32HostToLittle(sampleRate);
    [wavData appendBytes:&sampleRateLE length:4];
    uint32_t byteRateLE = CFSwapInt32HostToLittle(byteRate);
    [wavData appendBytes:&byteRateLE length:4];
    uint16_t blockAlignLE = CFSwapInt16HostToLittle(blockAlign);
    [wavData appendBytes:&blockAlignLE length:2];
    uint16_t bitsPerSampleLE = CFSwapInt16HostToLittle(bitsPerSample);
    [wavData appendBytes:&bitsPerSampleLE length:2];

    // data chunk
    [wavData appendBytes:"data" length:4];
    uint32_t dataSizeLE = CFSwapInt32HostToLittle(dataSize);
    [wavData appendBytes:&dataSizeLE length:4];

    void *silence = calloc(dataSize, 1);
    if (silence) {
        [wavData appendBytes:silence length:dataSize];
        free(silence);
    }

    [wavData writeToFile:silentPath atomically:YES];
    NSLog(@"[IOSAudioSession] Created silent audio file at: %@", silentPath);
}

- (void)startKeepAliveAudio {
    runOnMainThreadAsync(^{
        if (self->_silentPlayer && self->_silentPlayer.isPlaying) {
            return;
        }

        NSString *tempDir = NSTemporaryDirectory();
        NSString *silentPath = [tempDir stringByAppendingPathComponent:@"silence_48k.wav"];
        if (![[NSFileManager defaultManager] fileExistsAtPath:silentPath]) {
            [self createSilentAudioFile];
        }
        NSURL *silentURL = [NSURL fileURLWithPath:silentPath];

        NSError *error = nil;
        self->_silentPlayer = [[AVAudioPlayer alloc] initWithContentsOfURL:silentURL error:&error];

        if (error || !self->_silentPlayer) {
            NSLog(@"[IOSAudioSession] Error creating silent player: %@", error.localizedDescription);
            return;
        }

        self->_silentPlayer.numberOfLoops = -1;
        self->_silentPlayer.volume = 0.0001;

        if ([self->_silentPlayer play]) {
            NSLog(@"[IOSAudioSession] Keep-alive audio started");
        } else {
            NSLog(@"[IOSAudioSession] Failed to start keep-alive audio");
        }
    });
}

- (void)stopKeepAliveAudio {
    runOnMainThreadAsync(^{
        if (self->_silentPlayer) {
            [self->_silentPlayer stop];
            self->_silentPlayer = nil;
            NSLog(@"[IOSAudioSession] Keep-alive audio stopped");
        }
    });
}

- (void)setupAVAudioSession {
    [self startBackgroundTask];
    if (_isConnected) {
        [self startKeepAliveAudio];
    }
}

- (void)setupBackgroundAudio {
    [self startBackgroundTask];
    if (_isConnected) {
        [self startKeepAliveAudio];
    }
}

- (void)startBackgroundTask {
    runOnMainThreadAsync(^{
        [self stopBackgroundTask];

        self->_bgTask = [[UIApplication sharedApplication] beginBackgroundTaskWithName:@"DroidStarAudio"
                                                                      expirationHandler:^{
            runOnMainThreadAsync(^{
                NSLog(@"[IOSAudioSession] Background task expiring - renewing");
                if (self->_bgTask != UIBackgroundTaskInvalid) {
                    [[UIApplication sharedApplication] endBackgroundTask:self->_bgTask];
                    self->_bgTask = UIBackgroundTaskInvalid;
                }
                if (self->_isConnected) {
                    [self startBackgroundTask];
                }
            });
        }];
    });
}

- (void)stopBackgroundTask {
    runOnMainThreadAsync(^{
        if (self->_bgTask != UIBackgroundTaskInvalid) {
            [[UIApplication sharedApplication] endBackgroundTask:self->_bgTask];
            self->_bgTask = UIBackgroundTaskInvalid;
        }
    });
}

- (void)handleAppDidEnterBackground:(NSNotification *)notification {
    g_appInBackground.store(true, std::memory_order_relaxed);
    NSLog(@"[IOSAudioSession] App entered background");
    [self setupBackgroundAudio];
}

- (void)handleAppWillEnterForeground:(NSNotification *)notification {
    g_appInBackground.store(false, std::memory_order_relaxed);
    NSLog(@"[IOSAudioSession] App entering foreground");
}

- (void)handleAppDidBecomeActive:(NSNotification *)notification {
    g_appInBackground.store(false, std::memory_order_relaxed);
    NSLog(@"[IOSAudioSession] App became active");
}

- (void)handleAppWillResignActive:(NSNotification *)notification {
    NSLog(@"[IOSAudioSession] App will resign active");
    if (_isConnected) {
        [self startKeepAliveAudio];
    }
}

@end

extern "C" void setupAVAudioSession() {
    [[IOSAudioSession sharedInstance] setupAVAudioSession];
}

extern "C" void renewBackgroundTask() {
    [[IOSAudioSession sharedInstance] startBackgroundTask];
}

extern "C" bool isAppInBackground() {
    return g_appInBackground.load(std::memory_order_relaxed);
}

