/*
 *  TIM Voip Credential + Proxy address
 *
 *  compile with  g++ -std=c++0x -o voip voip.cpp -lresolv -lcurl -ljsoncpp
 *
 *  Author: czz78
 */

#include <arpa/nameser.h>
#include <arpa/inet.h>
#include <functional>
#include <iostream>
#include <netinet/in.h>
#include <resolv.h>
#include <string.h>
#include <map>
#include <curl/curl.h>
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/writer.h>
#include <jsoncpp/json/value.h>

using namespace std;


/*
 * Get the SRV record with priority 10 of domain
 */
int getTargetFromSRV(const char *domain) {

    // Ricordati di settare il dns telecom
    const char *dns1;
    const char *dns2;

    dns1 = "151.99.0.100";
    dns2 = "151.99.125.1";
    res_init();

    _res.nscount = 2;
    _res.nsaddr_list[0].sin_family = AF_INET;
    _res.nsaddr_list[0].sin_addr.s_addr = inet_addr (dns1);
    _res.nsaddr_list[0].sin_port = htons(53);
    _res.nsaddr_list[1].sin_family = AF_INET;
    _res.nsaddr_list[1].sin_addr.s_addr = inet_addr (dns2);
    _res.nsaddr_list[1].sin_port = htons(53);

    int response;
    unsigned char query_buffer[1024];
    {
        ns_type type;
        type= ns_t_srv;

        response= res_query(domain, C_IN, type, query_buffer, sizeof(query_buffer));
        if (response < 0) {
            std::cerr << "Error looking up service: '" << domain << "'" << std::endl;
            return 2;
        }
    }

    ns_msg nsMsg;
    ns_initparse(query_buffer, response, &nsMsg);

    map<ns_type, function<void (const ns_rr &rr)>> callbacks;

    callbacks[ns_t_srv]= [&nsMsg](const ns_rr &rr) -> void {
        char name[1024];
        dn_expand(ns_msg_base(nsMsg), ns_msg_end(nsMsg), 
                ns_rr_rdata(rr) + 6, name, sizeof(name));

        /* Check if priority is 10 */
        if(ntohs(*(unsigned short*)ns_rr_rdata(rr)) == 10) {
            std::cout << name << std::endl;
        }

    };


    for(int x= 0; x < ns_msg_count(nsMsg, ns_s_an); x++) {
        ns_rr rr;
        ns_parserr(&nsMsg, ns_s_an, x, &rr);
        ns_type type= ns_rr_type(rr);
        if (callbacks.count(type)) {
            callbacks[type](rr);
        }
    }

    return 0;

}


/*
 * Curl writer callback
 */
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}


/*
 * Get Json data from telecom
 * URl the url string
 * REQ could be GET or POST
 */
std::string downloadJSON(std::string URL, std::string REQ = "GET",  const char *PAYLOAD="") {

    std::string readBuffer;
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers=NULL; // init to NULL is important 

    headers= curl_slist_append( headers, "Content-Type: application/json");
    headers= curl_slist_append( headers, "Accept: application/json");
    headers= curl_slist_append( headers, "charsets: utf-8"); 
    headers= curl_slist_append( headers, "User-Agent: Dalvik/2.1.0 (Linux; U; Android 5.0.2; P10 Build/LRX22G)");
    headers= curl_slist_append( headers, "Connection: Keep-Alive");
    headers= curl_slist_append( headers, "Accept-Encoding: identity");
    curl = curl_easy_init();

    if (curl) {
        if(REQ == "POST") {
           //curl_easy_setopt(curl,CURLOPT_POST,1L );
           curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(PAYLOAD));   // if we don't provide POSTFIELDSIZE, libcurl will strlen() by itself
           curl_easy_setopt(curl,CURLOPT_POSTFIELDS, PAYLOAD);
           curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
        }
        else {
           curl_easy_setopt(curl, CURLOPT_HTTPGET,1);
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC); 
        curl_easy_setopt(curl, CURLOPT_USERPWD, "h9hkM75ymY5FjkQZ:Bs7yD-mu-NndwC!+5M39dY!nHRbEKjhs");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); 

        res = curl_easy_perform(curl);

        if (CURLE_OK == res) {
            char *ct;
            res = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);
            if((CURLE_OK == res) && ct) {
                curl_easy_cleanup(curl);
                return readBuffer;
            }


        }
    }

    curl_easy_cleanup(curl);
    return "";
}



/*
 *  Main
 */
int main(int argc, char *argv[]) {

    if (argc < 3) {
        std::cerr << std::endl << "Usage: " << argv[0] << " [voipline] [cellphone] [device_name]" << std::endl << std::endl;
        std::cerr << "      Example: ./voip 0434999999 3319999999 iphone"<< std::endl << std::endl;
        return 1;
    }

    std::string voipline(argv[1]);
    std::string cellphone(argv[2]);
    std::string device_name(argv[3]);

    std::string uid = std::to_string(rand() % 32767 + 10000);

    std::string  regmanurl1 = "https://regman-telefono.interbusiness.it:10800/AppTelefono/AnyModem/wsTIphoneProvisioning/v1/otpProvisioningSessions?CLI=%2B39"+voipline+"&DeviceId="+uid+"&Mobile=%2B39"+cellphone+"";
    std::string  regmanurl2 = "https://regman-telefono.interbusiness.it:10800/AppTelefono/AnyModem/wsTIphoneProvisioning/v1/associations";

    // get json data
    std::string data;
    data= downloadJSON(regmanurl1);

    if(data == "") {
      std::cout << "Error: could not retrieve " << regmanurl1 << std::endl;
      return 1;
    }

    Json::Value root;
    Json::Reader reader;
    bool parsingSuccessful = reader.parse( data.c_str(), root );     //parse process
    if ( !parsingSuccessful )
    {
        std::cout  << "Failed to parse" << reader.getFormattedErrorMessages();
        return 0;
    }

    // if cli has no value
    if(root.get("CLI", "null" ).asString() == "null" || root.get("CLI", "null" ).asString() == ""){
        std::cout << "Could not register phone, maybe you should deregister your app first or use another cell phone number" << std::endl;
        return 1;
    }

    std::string otp;
    // ask to insert OTP code received through sms
    std::cout << "Insert OTP received through sms: ";
    std::cin >> otp;

    // make payload
    std::string payload = "{ \"CLI\": \"" + root.get("CLI", "" ).asString() + "\" , \"DeviceId\": \"" + uid + "\" , \"DeviceName\": \"" + device_name + "\", \"Mobile\": \"+39" + cellphone + "\" , \"NotificationId\": \"android:ctHDua"+ uid +":BPA91bEvIgToivB52yRJ8_ati5sP5DSE4j2SnjUYG1dHRSetM0G6jqd" + uid + "FmhlP3zq7QlVAnYluf2yqbh-6lHPFWuzzq4BoqjgE4A-VnRo3xWmmFIrJYal5Hd3jyLRWCpTwh7TzW1_\", \"OTP\": \"" + otp + "\",\"SessionOTP\": \"" + root.get("SessionOTP", "" ).asString() + "\", \"TGU\": \"" + root.get("TGU", "" ).asString() + "\"}";
    //std::string payload = "CLI=" +  root.get("CLI", "" ).asString()  +"&DeviceId="+  uid  +"&DeviceName="+  device_name  +"&Mobile=+39"+  cellphone  +"&NotificationId=android:ctHDua"+uid+":BPA91bEvIgToivB52yRJ8_ati5sP5DSE4j2SnjUYG1dHRSetM0G6jqd"+uid+"FmhlP3zq7QlVAnYluf2yqbh-6lHPFWuzzq4BoqjgE4A-VnRo3xWmmFIrJYal5Hd3jyLRWCpTwh7TzW1_&OTP="+  otp  +"&SessionOTP="+  root.get("SessionOTP", "" ).asString()  +"&TGU="+  root.get("TGU", "" ).asString()  +"";
    const char *p_char = payload.c_str();

    data= downloadJSON(regmanurl2, "POST", p_char);

    if(data == "") {
      std::cout << "Error: could not retrieve " << regmanurl2 << std::endl;
      return 1;
    }


    parsingSuccessful = reader.parse( data.c_str(), root );     //parse process
    if ( !parsingSuccessful )
    {
        std::cout  << "Failed to parse" << reader.getFormattedErrorMessages();
        return 0;
    }


    /* Print */
    std::cout <<  " ====== VOIP CREDENTIAL ==========" << endl << endl;

    std::cout << "Pretty-Print: " << root.toStyledString() << std::endl;
    std::cout  << std::endl;

    std::cout <<  " ====== PROXY ADDRESS =========="  << std::endl << std::endl;

    std::string PCSCF_Address = root["ManagementObject"].get("P-CSCF_Address", "xxxxxx.co.imsw.telecomitalia.it" ).asString();

    if(PCSCF_Address != "xxxxxx.co.imsw.telecomitalia.it") {
        std::string dns_url_string = "_sip._udp." + PCSCF_Address;
        const char *dns_url = dns_url_string.c_str();
        getTargetFromSRV(dns_url);
    }
    else {
        std::cout << "No proxy address found" << std::endl;
    }

    std::cout << std::endl;


    return 0;
}

