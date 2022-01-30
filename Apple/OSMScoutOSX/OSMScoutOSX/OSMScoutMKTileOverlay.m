//
//  OSMScoutMKTileOverlay.m
//  MyFreemap
//
//  Created by Vladimir Vyskocil on 06/11/2013.
//  Copyright (c) 2013 Bleuazur. All rights reserved.
//

#import "OSMScoutMKTileOverlay.h"

#define RENDER_QUEUE_MAX_LEN 80

@implementation OSMScoutMKTileOperation

-(id)initWithOsmScout: (OSMScout *)osmScout x:(NSUInteger)x y:(NSUInteger)y zoom:(NSInteger)zoom contentScaleFactor:(CGFloat)contentScaleFactor result: (OSMScoutMKTileOperationCB)result {
    if ((self = [super init])) {
        executing = NO;
        finished = NO;
        _osmScout = osmScout;
        _x = x;
        _y = y;
        _zoom = zoom;
        _contentScaleFactor = contentScaleFactor;
        _result = [result copy];
    }
    return self;
}

- (BOOL)isConcurrent {
    return YES;
}

- (BOOL)isExecuting {
    return executing;
}

- (BOOL)isFinished {
    return finished;
}

- (void)start {
    // Always check for cancellation before launching the task.
    if ([self isCancelled]) {
        // Must move the operation to the finished state if it is canceled.
        [self willChangeValueForKey:@"isFinished"];
        finished = YES;
        [self didChangeValueForKey:@"isFinished"];
        return;
    }
    
    // If the operation is not canceled, begin executing the task.
    [self willChangeValueForKey:@"isExecuting"];
    [NSThread detachNewThreadSelector:@selector(main) toTarget:self withObject:nil];
    executing = YES;
    [self didChangeValueForKey:@"isExecuting"];
}

- (void)completeOperation {
    [self willChangeValueForKey:@"isFinished"];
    [self willChangeValueForKey:@"isExecuting"];
    
    executing = NO;
    finished = YES;
    
    [self didChangeValueForKey:@"isExecuting"];
    [self didChangeValueForKey:@"isFinished"];
}

-(void)main{
    
    @try {
#if TARGET_OS_IPHONE
        CGFloat contentScale;
        if (@available(iOS 15, *)) {
            contentScale = _contentScaleFactor;
        } else {
            contentScale = 1;
        }
        UIGraphicsBeginImageContextWithOptions(CGSizeMake(kOSMScoutDefaultTileSize, kOSMScoutDefaultTileSize), YES, contentScale);
        CGContextRef cg = UIGraphicsGetCurrentContext();
        [_osmScout drawMapTo:cg x:_x y:_y zoom:1<<_zoom width:kOSMScoutDefaultTileSize height:kOSMScoutDefaultTileSize];
        UIImage* img = UIGraphicsGetImageFromCurrentImageContext();
        NSData *imgData = UIImagePNGRepresentation(img);
        UIGraphicsEndImageContext();
        _result(imgData,nil);
#else
        CGContextRef bitmapContext = CGBitmapContextCreate(NULL, kOSMScoutDefaultTileSize, kOSMScoutDefaultTileSize, 8, 0, [[NSColorSpace genericRGBColorSpace] CGColorSpace], kCGBitmapByteOrder32Host|kCGImageAlphaPremultipliedFirst);
        NSGraphicsContext *nsgc = [NSGraphicsContext graphicsContextWithGraphicsPort:bitmapContext flipped:YES];
        [NSGraphicsContext setCurrentContext:nsgc];
        CGContextRef cg = [nsgc graphicsPort];
        CGAffineTransform flipVertical = CGAffineTransformMake(1, 0, 0, -1, 0, kOSMScoutDefaultTileSize);
        CGContextConcatCTM(cg, flipVertical);
        [_osmScout drawMapTo:cg x:_x y:_y zoom:1<<_zoom width:kOSMScoutDefaultTileSize height:kOSMScoutDefaultTileSize];
        CGImageRef cgImage = CGBitmapContextCreateImage(bitmapContext);
        CGContextRelease(bitmapContext);
        NSBitmapImageRep *bitmapRep = [[NSBitmapImageRep alloc] initWithCGImage:cgImage];
        NSDictionary *props = [NSDictionary dictionary];
        NSData *imgData = [bitmapRep representationUsingType:NSPNGFileType properties:props];
        _result(imgData,nil);
        CGImageRelease(cgImage);
#endif
        [self completeOperation];
    }
    @catch(...) {
        // Do not rethrow exceptions.
    }
}


@end

#pragma mark -

@implementation OSMScoutMKTileOverlay
static NSString* _path;
static OSMScout* _osmScout;
static NSOperationQueue *_drawQueue;

+(NSString *)path {
    return _path;
}

+(void) setPath:(NSString *)path {
    _path = [path copy];
}

+(OSMScout *)osmScout {
    if (_osmScout == nil) {
        double scale = 1;
        NSInteger dpi = 163;
#if TARGET_OS_IPHONE
        if (UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPad){
            dpi = 132;
        }
        scale = UIScreen.mainScreen.scale;
#else
        dpi = 220;
#endif
        dpi *= scale;
        _osmScout = [OSMScout OSMScoutWithPath:OSMScoutMKTileOverlay.path dpi:dpi];
    }
    
    return _osmScout;
}

+(NSOperationQueue *)drawQueue {
    if (_drawQueue == nil) {
        _drawQueue = [[NSOperationQueue alloc] init];
        [_drawQueue setMaxConcurrentOperationCount:1];
    }
    return _drawQueue;
}

-(id)initWithURLTemplate: (NSString *)urlTemplate {
    self = [super initWithURLTemplate:urlTemplate];
    if(self){
        self.tileSize = CGSizeMake(kOSMScoutDefaultTileSize, kOSMScoutDefaultTileSize);
        self.canReplaceMapContent = YES;
        self.minimumZ = 1;
        self.maximumZ = 21;
        self.geometryFlipped = YES;
    }
    return self;
}

- (void)loadTileAtPath:(MKTileOverlayPath)tilePath result:(void (^)(NSData *tileData, NSError *error))result {
    NSInteger tileY = ((1<<tilePath.z) - 1) - tilePath.y;
    OSMScoutMKTileOperation *drawOp = [[OSMScoutMKTileOperation alloc] initWithOsmScout: OSMScoutMKTileOverlay.osmScout x:tilePath.x y:tileY zoom:tilePath.z contentScaleFactor:tilePath.contentScaleFactor result:result];
    [OSMScoutMKTileOverlay.drawQueue addOperation:drawOp];
}

@end
