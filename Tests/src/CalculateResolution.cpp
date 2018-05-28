#include <cstddef>
#include <iostream>

#include <osmscout/util/File.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/Number.h>

#include <osmscout/system/Math.h>

int main(int /*argc*/, char* /*argv*/[])
{
  osmscout::Distance earthPerimeter=osmscout::GetEllipsoidalDistance(0.0,0.0,180.0,0.0);

  std::cout << "Halfway round the world: " << earthPerimeter.As<osmscout::Kilometer>() << " km" << std::endl;

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

    osmscout::Distance eqDistanceLat=osmscout::GetEllipsoidalDistance(0.0,0.0,0.0,latGranularity);
    osmscout::Distance eqDistanceLon=osmscout::GetEllipsoidalDistance(0.0,0.0,lonGranularity,0.0);
    osmscout::Distance eqDistance=osmscout::GetEllipsoidalDistance(0.0,0.0,lonGranularity,latGranularity);

    std::cout << "Min. equitorial distance: " << "Lat: " << eqDistanceLat.AsMeter() << "m" << " Lon: " << eqDistanceLon.AsMeter() << "m" << " both: " << eqDistance.AsMeter() << "m" << std::endl;

    osmscout::Distance dDistanceLat=osmscout::GetEllipsoidalDistance(7.465,51.514,
                                                         7.465,51.514+latGranularity);
    osmscout::Distance dDistanceLon=osmscout::GetEllipsoidalDistance(7.465,51.514,
                                                         7.465+lonGranularity,51.514);
    osmscout::Distance dDistance=osmscout::GetEllipsoidalDistance(7.465,51.514,
                                                      7.465+lonGranularity,51.514+latGranularity);

    std::cout << "Dortmund distance: " << "Lat: " << dDistanceLat.AsMeter() << "m" << " Lon: " << dDistanceLon.AsMeter() << "m" << " both: " << dDistance.AsMeter() << "m" << std::endl;

    osmscout::Distance pDistanceLat=osmscout::GetEllipsoidalDistance(0,90.0,
                                                         0,90.0-latGranularity);

    std::cout << "Pole distance: " << "Lat: " << pDistanceLat.AsMeter() << "m" << std::endl;

    std::cout << "----" << std::endl;
  }

  return 0;
}
