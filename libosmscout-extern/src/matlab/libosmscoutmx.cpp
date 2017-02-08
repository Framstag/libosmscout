/*
libosmscoutmx - a MATLAB mex function which uses libosmscout
Copyright (C) 2016 Transporter

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "libosmscoutmx.h"
#include <cctype>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdio.h>
#include <sstream>
#ifndef _WIN32
#include <strings.h>
#define stringcopy(target, size, source) strcpy(target, source)
#else
#define strcasecmp _stricmp
#define stringcopy(target, size, source) strcpy_s(target, size, source)
#define snprintf _snprintf
#endif
#include <osmscout/Database.h>
#include <osmscout/LocationService.h>
#include <osmscout/POIService.h>
#include <osmscout/TypeFeatures.h>
#include <osmscout/RoutingService.h>
#include <osmscout/RoutePostprocessor.h>
#include <osmscout/util/Geometry.h>
#ifdef OSMSCOUT_MAP_CAIRO
#include <osmscout/MapService.h>
#include <osmscout/MapPainterCairo.h>
#endif

#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
		switch (ul_reason_for_call)
		{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
		}
		return TRUE;
}
#endif

namespace data
{
	enum VALUE_TYPE
	{
		VT_STRING,
		VT_DOUBLE,
		VT_STRING_ARRAY,
		VT_DOUBLE_ARRAY
	};
	typedef struct
	{
		VALUE_TYPE type;
		std::string name;
		std::string value_string;
		double value_double;
		std::vector<std::string> value_string_array;
		std::vector<double> value_double_array;
	} PARAM;
	typedef std::vector<PARAM> STRUCTURE;

	PARAM parameter_string(std::string name, std::string value) { PARAM p = { VT_STRING, name, value, 0.0, std::vector<std::string>(), std::vector<double>() }; return p; }
	PARAM parameter_double(std::string name, double value) { PARAM p = { VT_DOUBLE, name, "", value, std::vector<std::string>(), std::vector<double>() }; return p; }
	PARAM parameter_bool(std::string name, bool value) { PARAM p = { VT_DOUBLE, name, "", value ? 1.0 : 0.0, std::vector<std::string>(), std::vector<double>() }; return p; }
	PARAM parameter_string_array(std::string name, std::vector<std::string> value) { PARAM p = { VT_STRING_ARRAY, name, "", 0.0, value, std::vector<double>() }; return p; }
	PARAM parameter_double_array(std::string name, std::vector<double> value) { PARAM p = { VT_DOUBLE_ARRAY, name, "", 0.0, std::vector<std::string>(), value }; return p; }

	mxArray* toMxArray(STRUCTURE structure)
	{
		mwSize dims[2] = { 1, 1 };
		char** field_names = new char*[structure.size()];
		int* field_number = new int[structure.size()];
		for (size_t uj = 0; uj < structure.size(); uj++)
		{
			size_t fnl = structure[uj].name.size();
			field_names[uj] = new char[fnl + 1];
			snprintf(field_names[uj], fnl + 1, structure[uj].name.c_str());
		}
		memset(field_number, 0, structure.size() * sizeof(int));
		mxArray* retVal = mxCreateStructArray(2, dims, (int)structure.size(), (const char**)field_names);
		for (size_t uj = 0; uj < structure.size(); uj++) field_number[uj] = mxGetFieldNumber(retVal, field_names[uj]);
		for (size_t uj = 0; uj < structure.size(); uj++)
		{
			if (structure[uj].type == VT_STRING)
				mxSetFieldByNumber(retVal, 0, field_number[uj], mxCreateString(structure[uj].value_string.c_str()));
			else
				mxSetFieldByNumber(retVal, 0, field_number[uj], mxCreateDoubleScalar(structure[uj].value_double));
		}
		for (size_t uj = 0; uj < structure.size(); uj++) delete field_names[uj];
		delete field_names;
		delete field_number;
		return retVal;
	}

	void toConsole(STRUCTURE structure)
	{
		size_t max_len = 0;
		for (size_t uj = 0; uj < structure.size(); uj++) max_len = std::max<size_t>(max_len, structure[uj].name.size());
		for (size_t uj = 0; uj < structure.size(); uj++)
		{
			mexPrintf("%s:", structure[uj].name.c_str());
			for (size_t uk = structure[uj].name.size(); uk < max_len + 1; uk++) mexPrintf(" ");
			if (structure[uj].type == VT_STRING)
				mexPrintf("%s\n", structure[uj].value_string.c_str());
			else
				mexPrintf("%.8f\n", structure[uj].value_double);
		}
	}
}

data::STRUCTURE LocationDescription(std::string map_directory, osmscout::GeoCoord location)
{
	osmscout::DatabaseParameter databaseParameter;
	osmscout::DatabaseRef database(new osmscout::Database(databaseParameter));
	if (!database->Open(map_directory.c_str()))
	{
		mexErrMsgTxt("Cannot open map database");
	}
	osmscout::LocationServiceRef locationService(std::make_shared<osmscout::LocationService>(database));
	osmscout::LocationDescription description;
	if (!locationService->DescribeLocation(location, description))
	{
		database->Close();
		mexErrMsgTxt("Error during generation of location description");
	}
	osmscout::LocationAtPlaceDescriptionRef atAddressDescription = description.GetAtAddressDescription();
	data::STRUCTURE structure;
	structure.push_back(data::parameter_double("lat", location.GetLat()));
	structure.push_back(data::parameter_double("lon", location.GetLon()));
	if (atAddressDescription)
	{
		structure.push_back(data::parameter_bool("place", atAddressDescription->IsAtPlace()));
		if (!atAddressDescription->IsAtPlace())
		{
			structure.push_back(data::parameter_double("distance", atAddressDescription->GetDistance()));
			structure.push_back(data::parameter_double("bearing", atAddressDescription->GetBearing()));
		}
		if (atAddressDescription->GetPlace().GetPOI())
		{
			if (atAddressDescription->GetPlace().GetPOI()->name.length() > 0) structure.push_back(data::parameter_string("poi", (char*)atAddressDescription->GetPlace().GetPOI()->name.c_str()));
		}
		std::string tmp;
		if (atAddressDescription->GetPlace().GetLocation() && atAddressDescription->GetPlace().GetAddress())
		{
			if (atAddressDescription->GetPlace().GetLocation()->name.size() > 0) tmp.append(atAddressDescription->GetPlace().GetLocation()->name);
			if (tmp.size() > 0 && atAddressDescription->GetPlace().GetAddress()->name.size() > 0) tmp.append(" ");
			if (atAddressDescription->GetPlace().GetAddress()->name.size() > 0) tmp.append(atAddressDescription->GetPlace().GetAddress()->name);
		}
		else if (atAddressDescription->GetPlace().GetLocation())
		{
			if (atAddressDescription->GetPlace().GetLocation()->name.size() > 0) tmp.append(atAddressDescription->GetPlace().GetLocation()->name);
		}
		else if (atAddressDescription->GetPlace().GetAddress())
		{
			if (atAddressDescription->GetPlace().GetAddress()->name.size() > 0) tmp.append(atAddressDescription->GetPlace().GetAddress()->name);
		}
		if (tmp.size() > 0) structure.push_back(data::parameter_string("address", (char*)tmp.c_str()));
		if (atAddressDescription->GetPlace().GetAdminRegion())
		{
			if (atAddressDescription->GetPlace().GetAdminRegion()->name.length() > 0) structure.push_back(data::parameter_string("admin_region", (char*)atAddressDescription->GetPlace().GetAdminRegion()->name.c_str()));
		}
	}
	database->Close();
	return structure;
}

typedef struct
{
	osmscout::GeoCoord coord;
	std::string name;
	std::string type;
} ds;

mxArray* dsLst2mxArray(std::vector<ds> list)
{
	const char* fieldnames[4] = { "Lat", "Lon", "Name", "Type" };
	int field_number[4];
	mwSize dims[2] = { 1, list.size() };
	mxArray* retVal = mxCreateStructArray(2, dims, 4, fieldnames);
	for (size_t uj = 0; uj < 4; uj++) field_number[uj] = mxGetFieldNumber(retVal, fieldnames[uj]);
	for (size_t uk = 0; uk < list.size(); uk++)
	{
		mxSetFieldByNumber(retVal, (int)uk, field_number[0], mxCreateDoubleScalar(list[uk].coord.GetLat()));
		mxSetFieldByNumber(retVal, (int)uk, field_number[1], mxCreateDoubleScalar(list[uk].coord.GetLon()));
		mxSetFieldByNumber(retVal, (int)uk, field_number[2], mxCreateString(list[uk].name.c_str()));
		mxSetFieldByNumber(retVal, (int)uk, field_number[3], mxCreateString(list[uk].type.c_str()));
	}
	return retVal;
}

void LookupPOI(std::string map_directory, osmscout::GeoBox boundingBox, std::list<std::string> typeNames, int nlhs, mxArray* plhs[])
{
	osmscout::DatabaseParameter databaseParameter;
	osmscout::DatabaseRef database(new osmscout::Database(databaseParameter));
	if (!database->Open(map_directory.c_str()))
	{
		mexErrMsgTxt("Cannot open map database");
	}
	osmscout::POIServiceRef	poiService(new osmscout::POIService(database));
	osmscout::TypeConfigRef typeConfig(database->GetTypeConfig());
	osmscout::TypeInfoSet nodeTypes(*typeConfig);
	osmscout::TypeInfoSet wayTypes(*typeConfig);
	osmscout::TypeInfoSet areaTypes(*typeConfig);
	osmscout::NameFeatureLabelReader nameLabelReader(*typeConfig);
	for (const auto &name : typeNames)
	{
		osmscout::TypeInfoRef type = typeConfig->GetTypeInfo(name);
		if ((bool)type)
		{
			if (type->GetIgnore())
			{
				mexPrintf("Cannot resolve type name '%s'\n", name.c_str());
				continue;
			}
			if (!type->GetIgnore())
			{
				if (type->CanBeNode()) nodeTypes.Set(type);
				if (type->CanBeWay()) wayTypes.Set(type);
				if (type->CanBeArea()) areaTypes.Set(type);
			}
		}
	}
	std::vector<osmscout::NodeRef> nodes;
	std::vector<osmscout::WayRef> ways;
	std::vector<osmscout::AreaRef> areas;
	if (!poiService->GetPOIsInArea(boundingBox, nodeTypes, nodes, wayTypes, ways, areaTypes, areas))
	{
		mexErrMsgTxt("Cannot load data from database");
	}
	std::vector<ds> node_data;
	std::vector<ds> way_data;
	std::vector<ds> area_data;
	size_t maxlenName = 0;
	size_t maxlenType = 0;
	for (const auto &node : nodes)
	{
		ds d;
		d.coord = node->GetCoords();
		d.name = nameLabelReader.GetLabel((node->GetFeatureValueBuffer()));
		d.type = node->GetType()->GetName();
		maxlenName = std::max<size_t>(maxlenName, d.name.size());
		maxlenType = std::max<size_t>(maxlenType, d.type.size());
		node_data.push_back(d);
	}
	for (const auto &way : ways)
	{
		ds d;
		way->GetCenter(d.coord);
		d.name = nameLabelReader.GetLabel((way->GetFeatureValueBuffer()));
		d.type = way->GetType()->GetName();
		maxlenName = std::max<size_t>(maxlenName, d.name.size());
		maxlenType = std::max<size_t>(maxlenType, d.type.size());
		way_data.push_back(d);
	}
	for (const auto &area : areas)
	{
		ds d;
		area->GetCenter(d.coord);
		d.name = nameLabelReader.GetLabel(area->rings.front().GetFeatureValueBuffer());
		d.type = area->GetType()->GetName();
		maxlenName = std::max<size_t>(maxlenName, d.name.size());
		maxlenType = std::max<size_t>(maxlenType, d.type.size());
		area_data.push_back(d);
	}
	if (nlhs == 0)
	{
		std::stringstream ss;
		if (node_data.size() > 0)
		{
			ss << "Nodes:" << std::endl << std::setprecision(8) << std::fixed;
			for (const auto &data : node_data)
			{
				ss << std::setw(13) << std::right << data.coord.GetLat() << " " << std::setw(13) << std::right << data.coord.GetLon() << " " << std::setw(maxlenName) << std::left << data.name.c_str() << " " << data.type.c_str() << std::endl;
			}
		}
		if (way_data.size() > 0)
		{
			ss << "Ways:" << std::endl << std::setprecision(8) << std::fixed;
			for (const auto &data : way_data)
			{
				ss << std::setw(13) << std::right << data.coord.GetLat() << " " << std::setw(13) << std::right << data.coord.GetLon() << " " << std::setw(maxlenName) << std::left << data.name.c_str() << " " << data.type.c_str() << std::endl;
			}
		}
		if (area_data.size() > 0)
		{
			ss << "Areas:" << std::endl << std::setprecision(8) << std::fixed;
			for (const auto &data : area_data)
			{
				ss << std::setw(13) << std::right << data.coord.GetLat() << " " << std::setw(13) << std::right << data.coord.GetLon() << " " << std::setw(maxlenName) << std::left << data.name.c_str() << " " << data.type.c_str() << std::endl;
			}
		}
		mexPrintf(ss.str().c_str());
	}
	else if (nlhs == 1)
	{
		const char* fieldnames[3] = { "Nodes", "Ways", "Areas" };
		plhs[0] = mxCreateStructMatrix(1, 1, 3, fieldnames);
		mxSetField(plhs[0], 0, fieldnames[0], dsLst2mxArray(node_data));
		mxSetField(plhs[0], 0, fieldnames[1], dsLst2mxArray(way_data));
		mxSetField(plhs[0], 0, fieldnames[2], dsLst2mxArray(area_data));
	}
	else if (nlhs == 3)
	{
		plhs[0] = dsLst2mxArray(node_data);
		plhs[1] = dsLst2mxArray(way_data);
		plhs[2] = dsLst2mxArray(area_data);
	}
	else if(nlhs == 5)
	{
		size_t count = node_data.size() + way_data.size() + area_data.size();
		plhs[0] = mxCreateDoubleMatrix(count, 1, mxREAL);
		double* pLat = mxGetPr(plhs[0]);
		plhs[1] = mxCreateDoubleMatrix(count, 1, mxREAL);
		double* pLon = mxGetPr(plhs[1]);
		size_t lenName = 0;
		size_t lenType = 0;
		size_t index = 0;
		for (size_t uj = 0; uj < node_data.size(); uj++)
		{
			lenName = std::max<size_t>(lenName, node_data[uj].name.size());
			lenType = std::max<size_t>(lenType, node_data[uj].type.size());
			pLat[index] = node_data[uj].coord.GetLat();
			pLon[index] = node_data[uj].coord.GetLon();
			index++;
		}
		for (size_t uj = 0; uj < way_data.size(); uj++)
		{
			lenName = std::max<size_t>(lenName, way_data[uj].name.size());
			lenType = std::max<size_t>(lenType, way_data[uj].type.size());
			pLat[index] = way_data[uj].coord.GetLat();
			pLon[index] = way_data[uj].coord.GetLon();
			index++;
		}
		for (size_t uj = 0; uj < area_data.size(); uj++)
		{
			lenName = std::max<size_t>(lenName, area_data[uj].name.size());
			lenType = std::max<size_t>(lenType, area_data[uj].type.size());
			pLat[index] = area_data[uj].coord.GetLat();
			pLon[index] = area_data[uj].coord.GetLon();
			index++;
		}
		char** tempName = new char*[count];
		char** tempType = new char*[count];
		char** tempNode = new char*[count];
		index = 0;
		for (size_t uj = 0; uj < node_data.size(); uj++)
		{
			tempName[index] = new char[lenName + 2];
			memset(tempName[index], 0, (lenName + 2) * sizeof(char));
			if (node_data[uj].name.size() > 0) snprintf(tempName[index], lenName + 1, node_data[uj].name.c_str());
			tempType[index] = new char[lenType + 2];
			memset(tempType[index], 0, (lenType + 2) * sizeof(char));
			if (node_data[uj].type.size() > 0) snprintf(tempType[index], lenType + 1, node_data[uj].type.c_str());
			tempNode[index] = new char[6];
			snprintf(tempNode[index], 6, "node");
			index++;
		}
		for (size_t uj = 0; uj < way_data.size(); uj++)
		{
			tempName[index] = new char[lenName + 2];
			memset(tempName[index], 0, (lenName + 2) * sizeof(char));
			if (way_data[uj].name.size() > 0) snprintf(tempName[index], lenName + 1, way_data[uj].name.c_str());
			tempType[index] = new char[lenType + 2];
			memset(tempType[index], 0, (lenType + 2) * sizeof(char));
			if (way_data[uj].type.size() > 0) snprintf(tempType[index], lenType + 1, way_data[uj].type.c_str());
			tempNode[index] = new char[6];
			snprintf(tempNode[index], 6, "way");
			index++;
		}
		for (size_t uj = 0; uj < area_data.size(); uj++)
		{
			tempName[index] = new char[lenName + 2];
			memset(tempName[index], 0, (lenName + 2) * sizeof(char));
			if (area_data[uj].name.size() > 0) snprintf(tempName[index], lenName + 1, area_data[uj].name.c_str());
			tempType[index] = new char[lenType + 2];
			memset(tempType[index], 0, (lenType + 2) * sizeof(char));
			if (area_data[uj].type.size() > 0) snprintf(tempType[index], lenType + 1, area_data[uj].type.c_str());
			tempNode[index] = new char[6];
			snprintf(tempNode[index], 6, "area");
			index++;
		}
		plhs[2] = mxCreateCharMatrixFromStrings(count, (const char **)tempName);
		plhs[3] = mxCreateCharMatrixFromStrings(count, (const char **)tempType);
		plhs[4] = mxCreateCharMatrixFromStrings(count, (const char **)tempNode);
		for (size_t uj = 0; uj < count; uj++)
		{
			delete tempName[uj];
			delete tempType[uj];
			delete tempNode[uj];
		}
		delete tempName;
		delete tempType;
		delete tempNode;
	}
	else
	{
		database->Close();
		mexErrMsgTxt("Wrong number of output arguments!");
	}
	database->Close();
}

#ifdef OSMSCOUT_MAP_CAIRO
void DrawMapCairo(std::string map_directory, std::string style_file, size_t width, size_t height, osmscout::GeoCoord location, double zoom, int nlhs, mxArray* plhs[], double DPI = 96.0)
{
	osmscout::DatabaseParameter databaseParameter;
	osmscout::DatabaseRef database(new osmscout::Database(databaseParameter));
	osmscout::MapServiceRef mapService(new osmscout::MapService(database));
	if (!database->Open(map_directory.c_str()))
	{
		mexErrMsgTxt("Cannot open map database");
	}
	osmscout::StyleConfigRef styleConfig(new osmscout::StyleConfig(database->GetTypeConfig()));
	if (!styleConfig->Load(style_file.c_str()))
	{
		database->Close();
		mexErrMsgTxt("Cannot open style file");
	}
	cairo_surface_t *surface;
	cairo_t         *cairo;
	surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);
	if (surface != NULL)
	{
		cairo = cairo_create(surface);
		if (cairo != NULL)
		{
			osmscout::MercatorProjection  projection;
			osmscout::MapParameter        drawParameter;
			osmscout::AreaSearchParameter searchParameter;
			osmscout::MapData             data;
			osmscout::MapPainterCairo     painter(styleConfig);
			drawParameter.SetFontSize(3.0);
			projection.Set(location, osmscout::Magnification(zoom), DPI, width, height);
			std::list<osmscout::TileRef> tiles;
			mapService->LookupTiles(projection, tiles);
			mapService->LoadMissingTileData(searchParameter, *styleConfig, tiles);
			mapService->AddTileDataToMapData(tiles, data);
			if (painter.DrawMap(projection, drawParameter, data, cairo))
			{
				cairo_surface_write_to_png(surface, "test.png");
				unsigned char* imgdata = cairo_image_surface_get_data(surface);
				int stride = cairo_image_surface_get_stride(surface);
				size_t dims[3] = { height, width, 3 };
				plhs[0] = mxCreateNumericArray(3, dims, mxUINT8_CLASS, mxREAL);
				if (imgdata == NULL)
				{
					cairo_destroy(cairo);
					cairo_surface_destroy(surface);
					database->Close();
					mexErrMsgTxt("Cannot get image data");
				}
				else
				{
					unsigned char* dest = (unsigned char *)mxGetData(plhs[0]);
					unsigned char* src = imgdata + width * (height - 1) * 4;
					size_t cb = width * height;
					for (int i = 0; i < height; i++)
					{
						for (int j = 0; j < width; j++)
						{
							dest[2 * cb + j * height + (height - i - 1)] = *src++;
							dest[1 * cb + j * height + (height - i - 1)] = *src++;
							dest[0 * cb + j * height + (height - i - 1)] = *src++;
							src++;
						}
						src -= width * 4 * 2;
					}
				}
			}
			cairo_destroy(cairo);
		}
		else
		{
			cairo_surface_destroy(surface);
			database->Close();
			mexErrMsgTxt("Cannot create cairo object");
		}
		cairo_surface_destroy(surface);
	}
	else
	{
		database->Close();
		mexErrMsgTxt("Cannot create cairo surface");
	}
	database->Close();
}
#endif

std::string help(bool output = true)
{
	std::stringstream ss;
	ss << "SYNTAX libosmscoutmx(...)" << std::endl << std::endl;
	ss << "EXAMPLES:" << std::endl;
	ss << "   libosmscoutmx('locationdescription', '../maps/nordrhein-westfalen', 51.57162, 7.45882)" << std::endl;
	ss << "   libosmscoutmx('poi', '../maps/nordrhein-westfalen', 51.2, 6.5, 51.7, 8, 'shop')" << std::endl;
	ss << "   libosmscoutmx('poi', '../maps/nordrhein-westfalen', 51.2, 6.5, 51.7, 8, {'amenity_hospital', 'amenity_hospital_building'})" << std::endl;
#ifdef OSMSCOUT_MAP_CAIRO
	ss << "   img = libosmscoutmx('map', '../maps/nordrhein-westfalen', '../stylesheets/standard.oss', 800, 600, 51.51241, 7.46525, 70000)" << std::endl;
#endif
	ss << std::endl << "DESCRIPTION:" << std::endl;
	ss << "'LocationDescription' '[map directory]' [lat] [lon]" << std::endl;
	ss << "   Get data about locations place or the place next to the location" << std::endl << std::endl;
	ss << "'POI' '[map directory]' [lat top] [lon left] [lat bottom] [lon right] '[type(s)]'" << std::endl;
	ss << "   Get data about POIs in selected area" << std::endl << std::endl;
#ifdef OSMSCOUT_MAP_CAIRO
	ss << "'Map' '[map directory]' '[style file]' [image width] [image height] [lat] [lon] [zoom]" << std::endl;
	ss << "   Draw a map and retreive a MATLAB image matrix" << std::endl << std::endl;
#endif
	if (output) mexPrintf(ss.str().c_str());
	return ss.str();
}

namespace arguments
{
	std::string index2string(int index)
	{
		switch (index)
		{
		case 1: return "1st";
		case 2: return "2nd";
		case 3: return "3rd";
		default:
		{
			char numbuf[16];
			snprintf(numbuf, 16, "%dth", index);
			return std::string(numbuf);
		}
		break;
		}
	}

	std::string read_string(int nrhs, mxArray* prhs[], int* index)
	{
		if (!mxIsChar(prhs[*index]))
		{
			char errmsg[256];
			snprintf(errmsg, 256, "The %s input argument must be a string!", index2string(*index + 1).c_str());
			mexErrMsgTxt(errmsg);
		}
		size_t len = std::max<size_t>(mxGetM(prhs[*index]), mxGetN(prhs[*index])) + 2;
		std::string text;
		text.resize(len);
		mxGetString(prhs[*index], (char*)text.c_str(), len - 1);
		text[len - 1] = 0;
		len = strlen(text.c_str());
		text.resize(len);
		*index = *index + 1;
		return text;
	}

	double read_double(int nrhs, mxArray* prhs[], int* index)
	{
		if (!mxIsNumeric(prhs[*index]))
		{
			char errmsg[256];
			snprintf(errmsg, 256, "The %s input argument must be a floating number!", index2string(*index + 1).c_str());
			mexErrMsgTxt(errmsg);
		}
		double* pdbl = mxGetPr(prhs[*index]);
		double retVal = pdbl[0];
		*index = *index + 1;
		return retVal;
	}

	std::vector<double> read_doubles(int nrhs, mxArray* prhs[], int* index, size_t count = 2)
	{
		std::vector<double> retVal;
		if (!mxIsNumeric(prhs[*index]))
		{
			char errmsg[256];
			snprintf(errmsg, 256, "The %s input argument must be a floating number!", index2string(*index + 1).c_str());
			mexErrMsgTxt(errmsg);
		}
		if (count == 1)
		{
			double* pdbl = mxGetPr(prhs[*index]);
			retVal.push_back(pdbl[0]);
			*index = *index + 1;
			return retVal;
		}
		size_t xrows = mxGetM(prhs[*index]);
		size_t xcols = mxGetN(prhs[*index]);
		if ((xrows == 1 && xcols == count) || (xrows == count && xcols == 1))
		{
			double* pdbl = mxGetPr(prhs[*index]);
			for (size_t uj = 0; uj < count; uj++) retVal.push_back(pdbl[uj]);
			*index = *index + 1;
		}
		else
		{
			for (size_t uj = 0; uj < count; uj++)
			{
				if (!mxIsNumeric(prhs[*index]))
				{
					char errmsg[256];
					snprintf(errmsg, 256, "The %s input argument must be a floating number!", index2string(*index + 1).c_str());
					mexErrMsgTxt(errmsg);
				}
				else
				{
					double* pdbl = mxGetPr(prhs[*index]);
					retVal.push_back(pdbl[0]);
				}
				*index = *index + 1;
			}
		}
		return retVal;
	}

	osmscout::GeoCoord read_location(int nrhs, mxArray* prhs[], int* index)
	{
		std::vector<double> location = read_doubles(nrhs, prhs, index, 2);
		if (location.size() != 2)
		{
			mexErrMsgTxt("Could not read a location (lat, lon)!");
		}
		return osmscout::GeoCoord(location[0], location[1]);
	}

	osmscout::GeoBox read_area(int nrhs, mxArray* prhs[], int* index)
	{
		std::vector<double> location = read_doubles(nrhs, prhs, index, 4);
		if (location.size() != 4)
		{
			mexErrMsgTxt("Could not read a bounding box (lat top, lon left, lat bottom, lon right)!");
		}
		return osmscout::GeoBox(osmscout::GeoCoord(std::min<double>(location[0], location[2]), std::min<double>(location[1], location[3])), osmscout::GeoCoord(std::max<double>(location[0], location[2]), std::max<double>(location[1], location[3])));
	}
}

/**
\brief Entry point to C/C++ or Fortran MEX-file
\param nlhs Number of expected output mxArrays
\param plhs Array of pointers to the expected output mxArrays
\param nrhs Number of input mxArrays
\param prhs Array of pointers to the input mxArrays. Do not modify any prhs values in your MEX-file. Changing the data in these read-only mxArrays can produce undesired side effects.
*/
void mexFunction(int nlhs, mxArray* plhs[], int nrhs, mxArray* prhs[])
{
	if (nrhs <= 0)
		help();
	else
	{
		if (!mxIsChar(prhs[0]))
		{
			mexErrMsgTxt("The first input argument must be a string with the command!");
		}
		size_t len = std::max<size_t>(mxGetM(prhs[0]), mxGetN(prhs[0])) + 2;
		std::string cmd;
		cmd.resize(len);
		mxGetString(prhs[0], (char*)cmd.c_str(), len - 1);
		cmd[len - 1] = 0;
		int index = 1;
		if (strcasecmp(cmd.c_str(), "LocationDescription") == 0)
		{
			std::string file = arguments::read_string(nrhs, prhs, &index);
			data::STRUCTURE structure = LocationDescription(file, arguments::read_location(nrhs, prhs, &index));
			if (nlhs == 1)
				plhs[0] = data::toMxArray(structure);
			else
				data::toConsole(structure);
		}
		else if (strcasecmp(cmd.c_str(), "POI") == 0)
		{
			std::string file = arguments::read_string(nrhs, prhs, &index);
			osmscout::GeoBox area = arguments::read_area(nrhs, prhs, &index);
			std::list<std::string> types;
			while (index < nrhs)
			{
				if (mxIsChar(prhs[index]))
				{
					std::string tmp = arguments::read_string(nrhs, prhs, &index);
					if (tmp.length() > 0) types.push_back(tmp);
				}
				else if (mxIsCell(prhs[index]))
				{
					mwSize total_num_of_cells = mxGetNumberOfElements(prhs[index]);
					for (mwIndex ci = 0; ci < total_num_of_cells; ci++)
					{
						const mxArray* cell_element_ptr = mxGetCell(prhs[index], ci);
						if (mxIsChar(cell_element_ptr))
						{
							size_t len = std::max<size_t>(mxGetM(cell_element_ptr), mxGetN(cell_element_ptr)) + 2;
							if (len <= 2) continue;
							std::string tmp;
							tmp.resize(len);
							mxGetString(cell_element_ptr, (char*)tmp.c_str(), len - 1);
							tmp[len - 1] = 0;
							len = strlen(tmp.c_str());
							tmp.resize(len);
							types.push_back(tmp);
						}
					}
					index++;
				}
			}
			LookupPOI(file, area, types, nlhs, plhs);
		}
#ifdef OSMSCOUT_MAP_CAIRO
		else if (strcasecmp(cmd.c_str(), "map") == 0)
		{
			if (nlhs != 1)
			{
				mexErrMsgTxt("There is only one output argument!");
			}
			std::string data = arguments::read_string(nrhs, prhs, &index);
			std::string style = arguments::read_string(nrhs, prhs, &index);
			std::vector<double> size = arguments::read_doubles(nrhs, prhs, &index, 2);
			osmscout::GeoCoord location = arguments::read_location(nrhs, prhs, &index);
			double zoom = arguments::read_double(nrhs, prhs, &index);
			DrawMapCairo(data, style, (size_t)size[0], (size_t)size[1], location, zoom, nlhs, plhs);
		}
#endif
		else
		{
			help();
		}
	}
}
