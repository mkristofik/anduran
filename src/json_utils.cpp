/*
    Copyright (C) 2016-2017 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#include "json_utils.h"

#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/prettywriter.h"

#include <cstdio>
#include <memory>

namespace
{
    const int JSON_BUFFER_SIZE = 65536;
}


rapidjson::Document jsonReadFile(const char *filename)
{
    rapidjson::Document doc;

    char buf[JSON_BUFFER_SIZE];
    std::shared_ptr<FILE> jsonFile(fopen(filename, "rb"), fclose);
    rapidjson::FileReadStream istr(jsonFile.get(), buf, sizeof(buf));
    doc.ParseStream(istr);

    return doc;
}

void jsonWriteFile(const char *filename, const rapidjson::Document &doc)
{
    using namespace rapidjson;

    char buf[JSON_BUFFER_SIZE];
    std::shared_ptr<FILE> jsonFile(fopen(filename, "wb"), fclose);
    FileWriteStream ostr(jsonFile.get(), buf, sizeof(buf));
    PrettyWriter<FileWriteStream> writer(ostr);

    writer.SetFormatOptions(kFormatSingleLineArray);
    doc.Accept(writer);
}
