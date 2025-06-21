#pragma once
#include <vector>
#include <utility>
#include <cmath>
#include <cstdint>

#include "AndroidInputBackend.h"

#ifdef ENABLE_GESTURE_TRACKING
inline GestureType classifyGesture(const std::vector<std::pair<float,float>>& start,
                                   const std::vector<std::pair<float,float>>& end){
  if(start.size()<2 || start.size()!=end.size())
    return GestureType::NONE;
  float dx0 = end[0].first - start[0].first;
  float dy0 = end[0].second - start[0].second;
  float dx1 = end[1].first - start[1].first;
  float dy1 = end[1].second - start[1].second;

  float startDist = std::hypot(start[1].first-start[0].first,
                               start[1].second-start[0].second);
  float endDist   = std::hypot(end[1].first-end[0].first,
                               end[1].second-end[0].second);
  float deltaDist = endDist-startDist;

  float ang0 = std::atan2(start[1].second-start[0].second,
                          start[1].first-start[0].first);
  float ang1 = std::atan2(end[1].second-end[0].second,
                          end[1].first-end[0].first);
  float deltaAng = ang1-ang0;

  float avgDx = (dx0+dx1)*0.5f;
  float avgDy = (dy0+dy1)*0.5f;
  float trans = std::hypot(avgDx,avgDy);

  if(std::fabs(deltaAng)>0.5f)
    return GestureType::ROTATE;
  if(std::fabs(deltaDist)>30.f)
    return deltaDist>0.f ? GestureType::PINCH_OUT : GestureType::PINCH_IN;
  if(trans>50.f)
    return GestureType::SWIPE;
  return GestureType::NONE;
}
#endif
