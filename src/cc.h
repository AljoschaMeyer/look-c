#ifndef OO_CC_H
#define OO_CC_H

#include "asg.h"
#include "rax.h"

// Removes items fom the asg whose cc (conditional compilation) attributes don't
// match the given set of features.
void oo_filter_cc(AsgFile *asg, rax *features);

#endif
