#include <cstddef>
#include <iostream>

#include <osmscout/util/File.h>
#include <osmscout/util/Geometry.h>

#include <osmscout/system/Math.h>

int main(int argc, char* argv[])
{
  // For all resolutions form 1 to 24 bits
  for (size_t i=1; i<=24; i++) {
    std::cout << "Resolutions: " << i << " bit(s)" << std::endl;

    unsigned int minValue=0;
    unsigned int maxValue=pow(2,i)-1;
    unsigned int bytesNeeded=osmscout::BytesNeededToAddressFileData(maxValue);

    std::cout << "Range: " << minValue << "-" << maxValue << " (" << bytesNeeded << " bytes)" << std::endl;

    double lonGranularity=360.0/maxValue;
    double latGranularity=180.0/maxValue;

    std::cout << "Lat granularity: " << latGranularity << ", Lon granularity: " << lonGranularity << std::endl;

    double eqDistanceLat=osmscout::GetEllipsoidalDistance(0.0,0.0,0.0,latGranularity);
    double eqDistanceLon=osmscout::GetEllipsoidalDistance(0.0,0.0,lonGranularity,0.0);
    double eqDistance=osmscout::GetEllipsoidalDistance(0.0,0.0,lonGranularity,latGranularity);

    std::cout << "Min. equitorial distance: " << "Lat: " << eqDistanceLat << "m" << " Lon: " << eqDistanceLon << "m" << " both: " << eqDistance << "m" << std::endl;

    double dDistanceLat=osmscout::GetEllipsoidalDistance(7.465,51.514,
                                                         7.465,51.514+latGranularity);
    double dDistanceLon=osmscout::GetEllipsoidalDistance(7.465,51.514,
                                                         7.465+lonGranularity,51.514);
    double dDistance=osmscout::GetEllipsoidalDistance(7.465,51.514,
                                                      7.465+lonGranularity,51.514+latGranularity);

    std::cout << "Dortmund distance: " << "Lat: " << dDistanceLat << "m" << " Lon: " << dDistanceLon << "m" << " both: " << dDistance << "m" << std::endl;

    double pDistanceLat=osmscout::GetEllipsoidalDistance(0,90.0,
                                                         0,90.0-latGranularity);

    std::cout << "Pole distance: " << "Lat: " << pDistanceLat << "m" << std::endl;

    std::cout << "----" << std::endl;
  }

  return 0;
}
