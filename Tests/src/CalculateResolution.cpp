#include <cstddef>
#include <iostream>

#include <osmscout/util/File.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/Number.h>

#include <osmscout/system/Math.h>

int main(int argc, char* argv[])
{
  double earthPerimeter=osmscout::GetEllipsoidalDistance(0.0,0.0,180.0,0.0);

  std::cout << "Halfway round the world: " << earthPerimeter << std::endl;

  // For all resolutions form 1 to 32 bits
  for (size_t i=1; i<=32; i++) {
    std::cout << "Resolutions: " << i << " bit(s)" << std::endl;

    unsigned int minValue=0;
    unsigned int maxValue=pow(2,i)-1;
    unsigned int bytesNeeded=osmscout::BytesNeededToEncodeNumber(maxValue);

    std::cout << "Range: " << minValue << "-" << maxValue << " (" << bytesNeeded << " bytes)" << std::endl;

    double lonGranularity=360.0/maxValue;
    double latGranularity=180.0/maxValue;

    std::cout << "Lat granularity: " << latGranularity << ", Lon granularity: " << lonGranularity << std::endl;

    double eqDistanceLat=osmscout::GetEllipsoidalDistance(0.0,0.0,0.0,latGranularity);
    double eqDistanceLon=osmscout::GetEllipsoidalDistance(0.0,0.0,lonGranularity,0.0);
    double eqDistance=osmscout::GetEllipsoidalDistance(0.0,0.0,lonGranularity,latGranularity);

    std::cout << "Min. equitorial distance: " << "Lat: " << eqDistanceLat*1000 << "m" << " Lon: " << eqDistanceLon*1000 << "m" << " both: " << eqDistance*1000 << "m" << std::endl;

    double dDistanceLat=osmscout::GetEllipsoidalDistance(7.465,51.514,
                                                         7.465,51.514+latGranularity);
    double dDistanceLon=osmscout::GetEllipsoidalDistance(7.465,51.514,
                                                         7.465+lonGranularity,51.514);
    double dDistance=osmscout::GetEllipsoidalDistance(7.465,51.514,
                                                      7.465+lonGranularity,51.514+latGranularity);

    std::cout << "Dortmund distance: " << "Lat: " << dDistanceLat*1000 << "m" << " Lon: " << dDistanceLon*1000 << "m" << " both: " << dDistance*1000 << "m" << std::endl;

    double pDistanceLat=osmscout::GetEllipsoidalDistance(0,90.0,
                                                         0,90.0-latGranularity);

    std::cout << "Pole distance: " << "Lat: " << pDistanceLat*1000 << "m" << std::endl;

    std::cout << "----" << std::endl;
  }

  return 0;
}
