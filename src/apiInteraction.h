#include "globals.h"
#include <cpr/cpr.h>
#include <iostream>
#include <string>
const std::string baseUrl = "https://lichess.org/api/";
const auto authHeader = cpr::Bearer(authTok);
void sendToLichess(std::string url, std::string payload1, std::string payload2);
