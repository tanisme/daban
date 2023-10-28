//
// file_handler.cpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "file_handler.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/bind/bind.hpp>
#include "mime_types.hpp"
#include "reply.hpp"
#include "request.hpp"
#include "LocalConfig.h"
#include "CApplication.h"
#include <json/json.h>

namespace http {
    namespace server4 {

        file_handler::file_handler(CApplication* app)
                : m_app(app) {
        }

        void file_handler::operator()(const request &req, reply &rep) {
            // Decode url to path.
            std::string request_path;
            if (!url_decode(req.uri, request_path)) {
                rep = reply::stock_reply(reply::bad_request);
                return;
            }

            // Request path must be absolute and not contain "..".
            if (request_path.empty() || request_path[0] != '/'
                || request_path.find("..") != std::string::npos) {
                rep = reply::stock_reply(reply::bad_request);
                return;
            }

            // If path ends in slash (i.e. is a directory) then add "index.html".
            if (request_path[request_path.size() - 1] == '/') {
                request_path += "index.html";
            }

            // Determine the file extension.
            std::size_t last_slash_pos = request_path.find_last_of("/");
            std::size_t last_dot_pos = request_path.find_last_of(".");
            std::string extension;
            if (last_dot_pos != std::string::npos && last_dot_pos > last_slash_pos) {
                extension = request_path.substr(last_dot_pos + 1);
            }

            Json::Reader reader;
            Json::FastWriter writer;
            Json::Value reqvalue, rspvalue, item, tmpvalue;

            m_app->m_ioc.post(boost::bind(&CApplication::OnFileHandle, m_app, 1));
            if (!reader.parse(req.content, reqvalue, false)) {
                return;
            }

            int ServiceNo = reqvalue["ServiceNo"].asInt();
            printf("ServiceNo %d\n", ServiceNo);

            if (ServiceNo == 1) {
                // 查询已经订阅合约
                for (auto &iter: LocalConfig::GetMe().m_mapSecurityID) {
                    item["SecurityID"] = iter.second.SecurityID;
                    //item["SecurityName"] = iter.second.SecurityName;
                    item["ExchangeID"] = iter.second.ExchangeID;
                    item["status"] = iter.second.Status;
                    tmpvalue.append(item);
                }
                rspvalue["security"] = tmpvalue;
            } else if (ServiceNo == 3) {
                // 增加订阅合约
                char Security[31] = {0};
                char ExchangeID = '0';
                strcpy(Security, reqvalue["SecurityID"].asString().c_str());
                ExchangeID = reqvalue["ExchangeID"].asString().c_str()[0];
                LocalConfig::GetMe().AddSubSecurity(Security, ExchangeID);
            }

            rspvalue["ServiceNo"] = ServiceNo + 1;
            rep.status = reply::ok;
            rep.content.append(writer.write(rspvalue));
            rep.headers.resize(2);
            rep.headers[0].name = "Content-Length";
            rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
            rep.headers[1].name = "Content-Type";
            rep.headers[1].value = mime_types::extension_to_type(extension);
        }

        bool file_handler::url_decode(const std::string &in, std::string &out) {
            out.clear();
            out.reserve(in.size());
            for (std::size_t i = 0; i < in.size(); ++i) {
                if (in[i] == '%') {
                    if (i + 3 <= in.size()) {
                        int value = 0;
                        std::istringstream is(in.substr(i + 1, 2));
                        if (is >> std::hex >> value) {
                            out += static_cast<char>(value);
                            i += 2;
                        } else {
                            return false;
                        }
                    } else {
                        return false;
                    }
                } else if (in[i] == '+') {
                    out += ' ';
                } else {
                    out += in[i];
                }
            }
            return true;
        }

    } // namespace server4
} // namespace http
