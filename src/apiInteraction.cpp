#include "apiInteraction.h"

void sendToLichess(std::string url) {
  long respCode = 404;

}

void sendToLichess(std::string url, std::string payload1, std::string payload2){
  long respCode = 404;
  if (!(payload1.empty() || payload2.empty())){
  while (respCode != 200){
    cpr::Response r = cpr::Post(cpr::Url{baseUrl+url}, authHeader, cpr::Payload{{payload1, payload2}});
    respCode = r.status_code;
    std::cerr << r.text << '\n';
  }} else {
  while (respCode != 200) {
    cpr::Response r = cpr::Post(cpr::Url{baseUrl + url}, authHeader);
    respCode = r.status_code;
    std::cerr << r.text << '\n';
  }}
}
