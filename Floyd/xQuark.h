//
//  Quark.h
//  Floyd
//
//  Created by Marcus Zetterquist on 2013-07-16.
//  Copyright (c) 2013 Marcus Zetterquist. All rights reserved.
//

#ifndef __Floyd__Quark__
#define __Floyd__Quark__

void OnAssert(const char iFileName[], int iLineNumber);

#define ASSERT_ON (DEBUG)
#define DEBUG 1
#define SCOPED_CHECK_INVARIANT
#define ASSERT(x) if(x){} else{ OnAssert(__FILE__, __LINE__); }
#define UT_VERIFY(x) if(x){} else{ OnAssert(__FILE__, __LINE__); }

#endif /* defined(__Floyd__Quark__) */
