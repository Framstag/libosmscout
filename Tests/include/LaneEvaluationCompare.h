#ifndef LIBOSMSCOUT_LANE_EVALUATION_COMPARE_H
#define LIBOSMSCOUT_LANE_EVALUATION_COMPARE_H

#include <string>

namespace osmscout {

struct CompareOptions
{
  std::string imagesDir;
  std::string stylesheet;
  std::string databaseDir;
};

int CompareRoutes(const std::string &oldPath,
                  const std::string &newPath,
                  const CompareOptions &options);

} // namespace osmscout

#endif // LIBOSMSCOUT_LANE_EVALUATION_COMPARE_H
