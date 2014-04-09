
#include "spinlock.h"

#include <atomic>

using utilities::spinlock;


spinlock::spinlock(): spin_(false)
{
};


spinlock::~spinlock()
{
};


void spinlock::enter()
{		
	while(spin_.exchange(true));
}


void spinlock::exit()
{
	spin_ = false;
}