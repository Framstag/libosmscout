#ifndef LIBOSMSCOUT_LANE_EVALUATION_COMPARE_H
#define LIBOSMSCOUT_LANE_EVALUATION_COMPARE_H

#include <cstddef>
#include <string>

namespace osmscout {

struct CompareOptions
{
  std::string imagesDir;
  std::string stylesheet;
  std::string databaseDir;
  size_t imageWidth=3840;
  size_t imageHeight=2160;
};

int CompareRoutes(const std::string &oldPath,
                  const std::string &newPath,
                  const CompareOptions &options);

} // namespace osmscout

#endif // LIBOSMSCOUT_LANE_EVALUATION_COMPARE_H
