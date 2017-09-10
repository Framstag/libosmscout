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

-(id)initWithOsmScout: (OSMScout *)osmScout x:(NSUInteger)x y:(NSUInteger)y zoom:(NSInteger)zoom scaleFactor: (CGFloat)scaleFactor result: (OSMScoutMKTileOperationCB)result {
    if((self = [super init])){
        executing = NO;
        finished = NO;
        _osmScout = osmScout;
        _x = x;
        _y = y;
        _zoom = zoom;
        _scaleFactor = scaleFactor;
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
    if ([self isCancelled])
    {
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
        UIGraphicsBeginImageContextWithOptions(CGSizeMake(kOSMScoutDefaultTileSize, kOSMScoutDefaultTileSize),YES,0);
        CGContextRef cg = UIGraphicsGetCurrentContext();
        CGFloat contentScale = [UIScreen mainScreen].scale;
        if(contentScale!=1.0){
          CGContextScaleCTM(cg, 1/contentScale, 1/contentScale);
        }
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
        CGAffineTransform flipVertical = CGAffineTransformMake(1, 0, 0, -1, 0, kOSMScoutDefaultTileSize*_scaleFactor);
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
@synthesize path = _path;

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

- (void)loadTileAtPath:(MKTileOverlayPath)path result:(void (^)(NSData *tileData, NSError *error))result {
    if(!_osmScout && _path){
        
        double scale = 1.0;
        NSInteger dpi = 163;
#if TARGET_OS_IPHONE
        if([[UIDevice currentDevice] respondsToSelector:@selector(userInterfaceIdiom)] &&
           UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad){
            dpi = 132;
        }
        if ([UIScreen instancesRespondToSelector:@selector(scale)]) {
            scale = [[UIScreen mainScreen] scale];
        }
#else
        dpi = 220;
#endif
        dpi *= scale;
        _osmScout = [OSMScout OSMScoutWithPath:_path dpi:dpi];
        _drawQueue = [[NSOperationQueue alloc] init];
        [_drawQueue setMaxConcurrentOperationCount:1];
    }
    
    OSMScoutMKTileOperation *drawOp = [[OSMScoutMKTileOperation alloc] initWithOsmScout: _osmScout x:path.x y:((1<<path.z) - 1)-path.y zoom:path.z scaleFactor:path.contentScaleFactor result:result];
    NSEnumerator *e = _drawQueue.operations.reverseObjectEnumerator;
    OSMScoutMKTileOperation *i;
    int count=0;
    while((i = [e nextObject])){
        if(!i.isFinished && !i.isCancelled){
            if(count<RENDER_QUEUE_MAX_LEN-1){
                [i addDependency:drawOp];
            } else {
                [i cancel];
            }
            count++;
        }
    }
    [_drawQueue addOperation:drawOp];
}

@end
