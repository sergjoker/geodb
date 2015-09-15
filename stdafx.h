// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"
#define FUSION_MAX_VECTOR_SIZE 20
#include <stdio.h>
#include <tchar.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <deque>
#include <fstream>
#include <iostream>
#include <iterator>
#include <list>
#include <queue>
#include <string>
#include <vector>

#include <boost/algorithm/cxx11/is_sorted.hpp>
#include <boost/config/warning_disable.hpp>
#include <boost/container/string.hpp>

#include <boost/flyweight.hpp>
#include <boost/flyweight/no_locking.hpp>
#include <boost/flyweight/no_tracking.hpp>
#include <boost/flyweight/set_factory.hpp>

#include <boost/foreach.hpp>

#include <boost/fusion/adapted/struct/define_struct_inline.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/define_struct.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/include/io.hpp>

#include <boost/lexical_cast.hpp>

#include <boost/range/iterator_range.hpp>
