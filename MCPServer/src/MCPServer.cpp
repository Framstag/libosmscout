/*
  MCPServer - a demo program for libosmscout
  Copyright (C) 2025  Tim Teulings

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

#include <fstream>
#include <string>
#include <vector>

#include <osmscout/cli/CmdLineParsing.h>

#include <osmscout/db/Database.h>

#include <osmscout/location/LocationService.h>
#include <osmscout/location/LocationDescriptionService.h>

#include <httplib.h>
#include <nlohmann/json.hpp>

#include "VersionTool.h"
#include "LocationDescriptionTool.h"

namespace {
  struct Arguments
  {
    bool                  help = false;
    std::string           databaseDirectory;
    std::filesystem::path dataPath;
    std::string           host="0.0.0.0";
    unsigned int          port=8000;

  };

  osmscout::CmdLineParseResult ParseArguments(const int argc, char** argv, Arguments& args)
  {
    osmscout::CmdLineParser argParser("MCPServer",
                                      argc, argv);
    const std::vector<std::string> helpArgs{"h", "help"};

    argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                          args.help=value;
                        }),
                        helpArgs,
                        "Return argument help",
                        true);

    argParser.AddOption(osmscout::CmdLineStringOption([&args](const std::string& value) {
                      args.host=value;
                    }),
                    "host",
                    "HTTP ip/hostname of server",
                    true);

    argParser.AddOption(osmscout::CmdLineUIntOption([&args](const unsigned int& value) {
                        args.port=value;
                      }),
                      "port",
                      "HTTP port of server",
                      true);

    argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string& value) {
                          args.dataPath=value;
                        }),
                        "DATA_PATH",
                        "Path of directory with MCP json files");

    argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string& value) {
                            args.databaseDirectory=value;
                          }),
                          "DATABASE",
                          "Directory of the db to use");

    osmscout::CmdLineParseResult result=argParser.Parse();
    if (result.HasError()) {
      osmscout::log.Error() << "ERROR: " << result.GetErrorDescription();
      osmscout::log.Info() << argParser.GetHelp();
    }
    else if (args.help) {
      osmscout::log.Info() << argParser.GetHelp();
    }

    return result;
  }

  std::string FileContentToString(const std::filesystem::path& path)
  {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    std::string content{std::istreambuf_iterator(file), std::istreambuf_iterator<char>()};
    file.close();

    return content;
  }

  void HandleMessageInitialize(const Arguments& args,
                               const nlohmann::json& reqObject,
                               httplib::Response& res)
  {
    unsigned int id = reqObject["id"];
    const std::string protocolVersion = reqObject["params"]["protocolVersion"];
    const std::string responseFile=osmscout::AppendFileToDir(args.dataPath,"capabilities.json");

    osmscout::log.Info() << "*** Message 'initialize' - id: " << id << " protocolVersion: " << protocolVersion;

    nlohmann::json resObject=nlohmann::json::parse(FileContentToString(responseFile));

    resObject.at("id")=id;

    res.set_content(resObject.dump(),"application/json");

    res.status=httplib::StatusCode::OK_200;
  }

  void HandleMessageInitialized(const Arguments& args,
                                const nlohmann::json& /*reqObject*/,
                                httplib::Response& res)
  {
    const std::string responseFile=osmscout::AppendFileToDir(args.dataPath,"initialized.json");

    osmscout::log.Info() << "*** Message 'initialized'";

    const nlohmann::json resObject=nlohmann::json::parse(FileContentToString(responseFile));

    res.set_content(resObject.dump(),"application/json");

    res.status=httplib::StatusCode::OK_200;
  }

  void HandleMessageToolsList(const Arguments& args,
                              const nlohmann::json& reqObject,
                              httplib::Response& res)
  {
    const std::string responseFile=osmscout::AppendFileToDir(args.dataPath,"tools.json");

    unsigned int id = reqObject["id"];

    osmscout::log.Info() << "*** Message 'tools/list' - id: " << id;

    nlohmann::json resObject=nlohmann::json::parse(FileContentToString(responseFile));

    resObject.at("id")=id;

    res.set_content(resObject.dump(),"application/json");

    res.status=httplib::StatusCode::OK_200;
  }

  void HandleMessageToolsCall(const nlohmann::json& reqObject,
                              httplib::Response& res,
                              const osmscout::LocationDescriptionServiceRef& locationDescriptionService)
  {
    unsigned int id=reqObject["id"];
    const std::string tool=reqObject["params"]["name"];

    osmscout::log.Info() << "*** Calling tool '" << tool << "'...";

    osmscout::mcp::ToolResult result;

    if (tool=="version") {
      result = osmscout::mcp::HandleVersion(id);
    }
    else if (tool=="locationDescription") {
      const nlohmann::json& arguments = reqObject["params"]["arguments"];
      result = osmscout::mcp::HandleLocationDescription(id, arguments, locationDescriptionService);
    }
    else {
      result.status = 400;
      result.body["jsonrpc"] = "2.0";
      result.body["id"] = id;
      result.body["error"]["message"] = "Unknown tool: " + tool;
    }

    res.set_content(result.body.dump(), "application/json");
    res.status = result.status;
  }

  void HandleMessagePing(const nlohmann::json& reqObject, httplib::Response& res)
  {
    unsigned int id = reqObject["id"];

    osmscout::log.Info() << "*** Message 'ping' - id: " << id;

    nlohmann::json resObject;

    resObject["jsonrpc"]="2.0";
    resObject["id"]= id;
    resObject["result"]=nlohmann::json::object();

    res.set_content(resObject.dump(),"application/json");

    res.status=httplib::StatusCode::OK_200;
  }
}

int main(const int argc, char* argv[])
{
  // Try to initialize current locale

  try {
    std::locale::global(std::locale(""));
  }
  catch (const std::runtime_error&) {
    osmscout::log.Error() << "ERROR: Cannot set locale";
  }

  Arguments args;

  if (const osmscout::CmdLineParseResult result=ParseArguments(argc, argv, args);
    result.HasError()) {
    osmscout::log.Error() << "Argument error";
    return 1;
  }

  if (args.help) {
    return 0;
  }

  osmscout::log.Info() << "Opening database at '" << args.databaseDirectory << "'...";

  const osmscout::DatabaseParameter databaseParameter;
  osmscout::DatabaseRef             database(new osmscout::Database(databaseParameter));

  if (!database->Open(args.databaseDirectory)) {
    osmscout::log.Error() << "Cannot open db";

    return 1;
  }

  auto locationService(std::make_shared<osmscout::LocationService>(database));
  const auto locationDescriptionService(std::make_shared<osmscout::LocationDescriptionService>(database));

  osmscout::log.Info() << "Creating server...";

  httplib::Server server;

  server.set_pre_routing_handler([](const auto& req, auto& /*res*/) {
    osmscout::log.Info() << "<<<| Server Request";
    osmscout::log.Info() << req.method << " " << req.path << " " << req.get_header_value("Content-Type");
    for (const auto& param : req.params) {
      osmscout::log.Info() << "Param: " << param.first << "=" << param.second;
    }
    for (const auto& header : req.headers) {
      osmscout::log.Info() << "Header " << header.first << ": " << header.second;
    }

    /*
    if (req.method=="POST") {
      osmscout::log.Info() << "---%<---";
      osmscout::log.Info() << req.body;
      osmscout::log.Info() << "---%<---";
    }*/

    osmscout::log.Info() << "|<<<";

    return httplib::Server::HandlerResponse::Unhandled;
  });

  server.set_post_routing_handler([](const auto& req, auto& res) {
    osmscout::log.Info() << "<<<| Server Response";
    osmscout::log.Info() << req.method << " " << req.path;
    for (const auto& header : res.headers) {
      osmscout::log.Info() << "Header " << header.first << ": " << header.second;
    }

    osmscout::log.Info() << "HTTP Status: " << res.status;

    osmscout::log.Info() << "---%<---";
    osmscout::log.Info() << res.body;
    osmscout::log.Info() << "---%<---";

    osmscout::log.Info() << "|<<<";

    return httplib::Server::HandlerResponse::Unhandled;
  });

  // OpenAPI served via mcpo proxy, no direct endpoint needed

  server.Post("/", [&](const httplib::Request& req, httplib::Response& res) {
    osmscout::log.Info() << "POST /";

    // TODO: Check content-type

    osmscout::log.Info() << "---%<---";
    osmscout::log.Info() << req.body;
    osmscout::log.Info() << "---%<---";

    const nlohmann::json reqObject = nlohmann::json::parse(req.body);
    const std::string method = reqObject["method"];

    if (method=="initialize") {
      HandleMessageInitialize(args,reqObject,res);
    }
    else if (method=="notifications/initialized") {
      HandleMessageInitialized(args,reqObject,res);
    }
    else if (method=="tools/list") {
      HandleMessageToolsList(args,reqObject,res);
    }
    else if (method=="tools/call") {
      HandleMessageToolsCall(reqObject,
                          res,
                             locationDescriptionService);
    }
    else if (method=="ping") {
      HandleMessagePing(reqObject,res);
    }
    else {
      osmscout::log.Info() << "*** Method: " << method;

      res.status=httplib::StatusCode::NotFound_404;
    }
  });

  osmscout::log.Info() << "Starting server on " << args.host << ":" << args.port << "...";
  server.listen(args.host, args.port);
  osmscout::log.Info() << "Starting stopped.";

  return 0;
}