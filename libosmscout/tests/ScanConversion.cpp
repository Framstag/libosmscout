#include <iostream>

#include <osmscout/util/Geometry.h>

int errors=0;

int main()
{
  std::vector<osmscout::ScanCell> cells;

  cells.clear();

  osmscout::ScanConvertLine(0,0,0,0,cells);

  if (cells.size()!=1) {
    std::cerr << "Wrong result count for simple point" << std::endl;
    errors++;
  }
  else if (cells[0].x!=0 ||cells[0].y!=0) {
    std::cerr << "Result for simple point has wrong coordinates" << std::endl;
    errors++;
  }

  cells.clear();

  osmscout::ScanConvertLine(0,0,1,1,cells);

  if (cells.size()!=2) {
    std::cerr << "Wrong result count for simple diagonal" << std::endl;
    errors++;
  }
  else if (cells[0].x!=0 ||cells[0].y!=0) {
    std::cerr << "Result for simple diagonal has wrong coordinates" << std::endl;
    errors++;
  }
  else if (cells[1].x!=1 ||cells[1].y!=1) {
    std::cerr << "Result for simple diagonal has wrong coordinates" << std::endl;
    errors++;
  }

  cells.clear();

  osmscout::ScanConvertLine(0,0,10,0,cells);

  if (cells.size()!=11) {
    std::cerr << "Wrong result count for simple horizontal" << std::endl;
    errors++;
  }
  else if (cells[0].x!=0 ||cells[0].y!=0) {
    std::cerr << "Result for simple horizontal has wrong coordinates" << std::endl;
    errors++;
  }
  else if (cells[10].x!=10 ||cells[10].y!=0) {
    std::cerr << "Result for simple horizontal has wrong coordinates" << std::endl;
    errors++;
  }

  cells.clear();

  osmscout::ScanConvertLine(0,0,0,10,cells);

  if (cells.size()!=11) {
    std::cerr << "Wrong result count for simple vertical" << std::endl;
    errors++;
  }
  else if (cells[0].x!=0 ||cells[0].y!=0) {
    std::cerr << "Result for simple vertical has wrong coordinates" << std::endl;
    errors++;
  }
  else if (cells[10].x!=0 ||cells[10].y!=10) {
    std::cerr << "Result for simple vertical has wrong coordinates" << std::endl;
    errors++;
  }

  if (errors>0) {
    return 1;
  }
  else {
    return 0;
  }
}
