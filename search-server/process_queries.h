//
// Created by rustam on 06.04.2022.
//
#pragma once

#include <algorithm>
#include <execution>

#include "search_server.h"

std::vector<std::vector<Document>> ProcessQueries(
        const SearchServer& search_server,
        const std::vector<std::string>& queries);
