// 인공지능에 관련된 선언들을 모아 놓는다.
#pragma once


#include "../Common/common.h"
//using namespace common;

// 부스트 메모리 풀 이용.
#include <boost/pool/object_pool.hpp>


#include "aidef.h"
#include "control/object.h"
#include "control/messagemanager.h"
#include "control/aimain.h"

#include "object/agent.h"
#include "action/action.h"
#include "action/rootaction.h"
#include "object/brain.h"

#include "action/move.h"

#include "path\pathfinder.h"
#include "path\pathfinder2d.h"
#include "path\navigationmesh.h"
