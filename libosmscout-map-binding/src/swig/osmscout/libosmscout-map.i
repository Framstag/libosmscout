%module libosmscoutmap

%include <typemaps.i>

%include <stdint.i>
%include <stl.i>

%include <std_string.i>
%include <std_wstring.i>
%include <std_shared_ptr.i>

%include <std_list.i>
%include <std_set.i>
%include <std_unordered_map.i>
//%include <std_unordered_set.i>
%include <std_vector.i>

%import <osmscout/libosmscout.i>

%include <osmscout/MapImportExport.i>

%include <osmscout/DataTileCache.i>
%include <osmscout/MapData.i>

%include <osmscout/StyleDescription.i>
%include <osmscout/Styles.i>
%include <osmscout/StyleProcessor.i>
%include <osmscout/MapParameter.i>
%include <osmscout/LabelProvider.i>
%include <osmscout/StyleConfig.i>

%include <osmscout/MapService.i>
