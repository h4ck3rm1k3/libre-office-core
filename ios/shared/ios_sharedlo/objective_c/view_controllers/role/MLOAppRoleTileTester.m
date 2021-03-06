// -*- Mode: ObjC; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
//
// This file is part of the LibreOffice project.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#import "MLOAppRoleTileTester.h"
#import "MLOMainViewController_Private.h"
#import "MLOTopbarViewController.h"
#import "MLOTestingTileRendererViewController.h"
#import "MLOTestingTileParametersViewController.h"


@implementation MLOAppRoleTileTester

-(void)initSubviews{
    NSLog(@"%@ initSubviews",self);
    self.renderer = [[MLOTestingTileRendererViewController alloc] initWithTester:self];
    self.params = [[MLOTestingTileParametersViewController alloc] initWithTester:self];
   
}

-(NSString *)description{
    return @"MLOAppRoleTileTester";
}


-(void)setWidth:(CGFloat) width height:(CGFloat) height{
    NSLog(@"%@ setWidth:height",self);
    if(width > height){
        CGFloat halfWidth = width/2.0f;
        self.renderer.view.frame =CGRectMake(0, 0, halfWidth, height);
        self.params.view.frame =CGRectMake(halfWidth,0, halfWidth, height);
    }else{
        CGFloat halfHeight = height/2.0f;
        self.params.view.frame =CGRectMake(0, 0, width, halfHeight);
        self.renderer.view.frame =CGRectMake(0,halfHeight, width, halfHeight);
    }
    [self.renderer resize];
    [self.params resize];

}
-(void)addSubviews{
    NSLog(@"%@ addSubviews",self);

    [self.renderer addToSuperview];
    [self.params addToSuperview];

    [self.mainViewController resize];
}

-(void)rotate{
    NSLog(@"%@ rotate",self);
    MLOMainViewController * main = [self mainViewController];
    
    [main resize];

    [main.topbar onRotate];

    [self.renderer resize];
    [self.params resize];
}


-(void)showLibreOffice{
    NSLog(@"%@ showLibreOffice",self);
}
-(void)hideLibreOffice{
    NSLog(@"%@ hideLibreOffice",self);
}

-(void)initWindow:(UIWindow *) window{
    NSLog(@"%@ initWindow",self);

}
@end
