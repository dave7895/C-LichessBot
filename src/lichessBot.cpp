// g++ main.cxx -I../libchesskz04px/src -lcurl -lcurlpp -o lichessBot
//  [dave@david-aspire C++LichessBot]$ ./lichessBot
#include "lichessBot.h"
int main(int argc, char const *argv[]) {
  /* code */
  std::string apiUrl = "https://lichess.org/api";
  auto eventUrl = cpr::Url{apiUrl + "/stream/event"};
  cpr::Response r =
      cpr::Get(cpr::Url{"https://lichess.org/api/account"}, authHeader);
  std::cout << r.status_code << std::endl;
  /*std::string paddedBody = r.text;
  paddedBody.reserve(paddedBody.length() + simdjson::SIMDJSON_PADDING);
  simdjson::ondemand::parser idParser;
  simdjson::ondemand::document doc = idParser.iterate(paddedBody);
  std::string myId2 = std::string{std::string_view{doc["id"]}};
  std::string g_myId = myId2;*/
  std::cout << "beginning request with WriteCallback\n";
  /*cpr::Response r2 = cpr::Get(
      eventUrl, authHeader,
      cpr::WriteCallback([](std::string data, intptr_t userdata) -> bool {
        std::cout << data.length() << "\n";
        std::cout << data << "\n";
        std::cout << "userdata: " << userdata << "\n";
        return true;
      }));*/

  // TODO: fill myId with my actual ID
  cpr::Response r3 =
      cpr::Get(eventUrl, authHeader, cpr::WriteCallback{streamEventsCallback});
  auto future_text = cpr::GetCallback(
      [](cpr::Response r) {
        std::cout << r.text << std::endl;
        return r.text;
      },
      eventUrl, authHeader);
  return 0;
}
