#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <vector>
#include <condition_variable>
#include <mutex>
#include "ThostFtdcMdApi.h"
#include "ThostFtdcTraderApi.h"
#include <spdlog/spdlog.h>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>  // ���� nlohmann json ��
#include <thread>

using json = nlohmann::json;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// ���ӿ�
#pragma comment (lib, "thostmduserapi_se.lib")
#pragma comment (lib, "thosttraderapi_se.lib")

// ģ�⾭���̴����Ͷ������Ϣ
TThostFtdcBrokerIDType gBrokerID = "RohonDemo"; // ����д��ȷ��BrokerID
TThostFtdcInvestorIDType gInvesterID = "ylzccs01"; // ylzccs01
TThostFtdcPasswordType gInvesterPassword = "888888"; // 888888

char frontAddressMd[] = "tcp://124.160.66.120:41253";
char frontAddressTrader[] = "tcp://210.22.96.58:18111"; // ����Ľ���ǰ�õ�ַ

class CustomMdSpi : public CThostFtdcMdSpi {
public:
    CustomMdSpi(CThostFtdcMdApi* pMdApi, CThostFtdcTraderApi* pTraderApi)
        : m_pMdApi(pMdApi), m_pTraderApi(pTraderApi), nRequestID(0) {}


    void OnFrontConnected() override {
        spdlog::info("������������ӳɹ�����¼��...");
        CThostFtdcReqUserLoginField reqUserLogin = { 0 };
        strncpy_s(reqUserLogin.BrokerID, gBrokerID, sizeof(reqUserLogin.BrokerID) - 1);
        strncpy_s(reqUserLogin.UserID, gInvesterID, sizeof(reqUserLogin.UserID) - 1);
        strncpy_s(reqUserLogin.Password, gInvesterPassword, sizeof(reqUserLogin.Password) - 1);
        
        int ret = m_pMdApi->ReqUserLogin(&reqUserLogin, nRequestID++);

        // Check �Ƿ����ӳɹ�
        if (ret == 0) {
            spdlog::info("�����¼�����ͳɹ�");
        } else {
            spdlog::error("�����¼������ʧ�ܣ�������: {}", ret);
        }
    }
    
    void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override {
        spdlog::info("�յ������¼��Ӧ");
        if (pRspInfo && pRspInfo->ErrorID == 0) {
            spdlog::info("�����¼�ɹ�>>>>>");
        }
        else {
            spdlog::error("�����¼ʧ��: {}", (pRspInfo ? pRspInfo->ErrorMsg : "δ֪����"));
            spdlog::error("�����û���������;����̴����Ƿ���ȷ��");
        }
    }
    

    CThostFtdcMdApi* m_pMdApi;
    CThostFtdcTraderApi* m_pTraderApi;
    int nRequestID;
};

class CustomTraderSpi : public CThostFtdcTraderSpi {
public:
    CustomTraderSpi(CThostFtdcTraderApi* pTraderApi)
        : m_pTraderApi(pTraderApi) {}

    void OnFrontConnected() override {

        // �ͻ�����֤����
        CThostFtdcReqAuthenticateField reqAuthenticate = { 0 };
        strncpy_s(reqAuthenticate.BrokerID, gBrokerID, sizeof(reqAuthenticate.BrokerID) - 1);
        strncpy_s(reqAuthenticate.UserID, gInvesterID, sizeof(reqAuthenticate.UserID) - 1);
        strncpy_s(reqAuthenticate.UserProductInfo, "YourProductInfo", sizeof(reqAuthenticate.UserProductInfo) - 1);
        strncpy_s(reqAuthenticate.AuthCode, "VHGgJNmQdEna2w4h", sizeof(reqAuthenticate.AuthCode) - 1);
        strncpy_s(reqAuthenticate.AppID, "liuc_client_0.1.0", sizeof(reqAuthenticate.AppID) - 1);

        int ret_Authenticate = m_pTraderApi->ReqAuthenticate(&reqAuthenticate, nRequestID++);
        if (ret_Authenticate == 0) {
            spdlog::info("�ͻ�����֤�����ͳɹ�");
        }
        else {
            spdlog::error("�ͻ�����֤������ʧ�ܣ�������: {}", ret_Authenticate);
        }

        spdlog::info("���׷��������ӳɹ�����¼��...");
        CThostFtdcReqUserLoginField reqUserLogin = { 0 };
        strncpy_s(reqUserLogin.BrokerID, gBrokerID, sizeof(reqUserLogin.BrokerID) - 1);
        strncpy_s(reqUserLogin.UserID, gInvesterID, sizeof(reqUserLogin.UserID) - 1);
        strncpy_s(reqUserLogin.Password, gInvesterPassword, sizeof(reqUserLogin.Password) - 1);

        int ret_UserLogin = m_pTraderApi->ReqUserLogin(&reqUserLogin, nRequestID++);

        // Check �Ƿ����ӳɹ�
        if (ret_UserLogin == 0) {
            spdlog::info("���׵�¼�����ͳɹ�");
        } else {
            spdlog::error("���׵�¼������ʧ�ܣ�������: {}", ret_UserLogin);
        }
    }
    void OnFrontDisconnected(int nReason) override {
        spdlog::info("���׷��������ӶϿ���ԭ��: {}", nReason);
        isLoggedIn = false; // ���µ�¼״̬
    }

    void OnRspAuthenticate(CThostFtdcRspAuthenticateField* pRspAuthenticateField, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override {
        spdlog::info("�յ��ͻ�����֤��Ӧ");
        if (pRspInfo && pRspInfo->ErrorID == 0) {
            spdlog::info("�ͻ�����֤�ɹ�>>>>>");
        }
        else {
            spdlog::info("�ͻ�����֤ʧ��: {}", (pRspInfo ? pRspInfo->ErrorMsg : "δ֪����"));
        }
    
    };
private:
    bool isLoggedIn = false; // ��ӵ�¼״̬��־
    #define USE_ALTERNATE_LOGIN_HANDLER


    void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override {
        spdlog::info("�յ����׵�¼��Ӧ");

        #if defined(USE_ALTERNATE_LOGIN_HANDLER)
            // Alternate login handler code
            //spdlog::info("���׵�¼ʧ��: {}", (pRspInfo ? pRspInfo->ErrorMsg : "δ֪����"));
            char currentDirection = THOST_FTDC_D_Buy;
            //PlaceMarketOrder(currentDirection);
            //QueryPositions();
            PlaceOrder(3200,2);
            QueryOrders();

            
        #else
            // Original version of the function (default case)

            if (pRspInfo && pRspInfo->ErrorID == 0) {
                spdlog::info("���׵�¼�ɹ�����ʼÿ���ӻ�ȡ�����źŲ��µ�");

                // ����һ���µ��߳���ÿ���Ӽ�齻���ź�
                std::thread([this]() {
                    while (true) {
                        CURL* curl;
                        CURLcode res;
                        std::string readBuffer;

                        // ��ʼ�� libcurl ��
                        curl_global_init(CURL_GLOBAL_DEFAULT);
                        curl = curl_easy_init();

                        if (curl) {

                            // ģ�͵� URL
                            //curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.4.235:5000/trade");

                            // ���Ե� URL ��http://127.0.0.1:5001
                            curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:5001/data");

                            // ���ûص�����������������
                            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

                            // ִ������
                            res = curl_easy_perform(curl);

                            // ��������Ƿ�ɹ�
                            if (res != CURLE_OK) {
                                std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
                            }
                            else {
                                //std::cout << "Response Data: " << readBuffer << std::endl;

                                // ���� JSON ����
                                try {
                                    json j = json::parse(readBuffer);
                                    std::string status = j["status"];
								    if (status != "success") {
									    spdlog::info("��Ч����Ӧ/����ʱ��: {}", status);
									    continue;
								    }

                                    json predictions = j["predictions"];
                                    std::string firstKeyStr = "pred_30_8_20";
                                    bool firstValue = predictions[firstKeyStr];


                                    // ��ӡ firstValue
                                    std::cout << "First key: " << firstKeyStr << ", First value: " << (firstValue ? "true" : "false") << std::endl;
                                    spdlog::info("First key: {}, First value: {}", firstKeyStr, (firstValue ? "true" : "false"));

                                    char currentDirection = 0;  // Default, no direction

                                    if (firstValue == true){
                                        currentDirection = THOST_FTDC_D_Buy;
                                    }
								    else if (firstValue == false){
									    currentDirection = THOST_FTDC_D_Sell;
									    }
								    else {
									    spdlog::info("�����⣬firstValue �Ȳ��� True Ҳ���� False}");
									    continue;
								    }

                                    // ��鵱ǰ�źŷ����Ƿ���Ҫ�µ�
                                    if (lastDirection == 0) {
                                        // ���û�гֲ֣�ֱ�Ӹ��ݵ�ǰ�źŷ����µ�
                                        std::cout << "�޳ֲ֣���һ��" << (firstValue == true ? "��" : "����") << std::endl;
                                        PlaceMarketOrder(currentDirection);
                                        lastDirection = currentDirection;  // ���³ֲַ���
                                    }
                                    else if (lastDirection != currentDirection) {
                                        // ����гֲֲ��ҷ����뵱ǰ�ź��෴����ƽ��
                                        std::cout << "�źŷ����򣺲�ѯ��λ��ƽ��" << std::endl;
                                        QueryPositions();

                                        // ƽ�ֺ���һ��һ����
                                        std::cout << "��һ��" << (firstValue == true ? "��" : "����") << std::endl;
                                        PlaceMarketOrder(lastDirection == THOST_FTDC_D_Buy ? THOST_FTDC_D_Sell : THOST_FTDC_D_Buy);
                                        lastDirection = currentDirection;  // ƽ�ֺ󽫳ֲַ�������Ϊ��
                                    }
                                    else if (lastDirection == currentDirection) {
                                        std::cout << "�ź���ͬ�������ֲ�" << std::endl;
                                    }

                                }
                                catch (json::parse_error& e) {
                                    spdlog::error("JSON ��������: {}", e.what());
                                }
                            }

                            // ����
                            curl_easy_cleanup(curl);
                        }

                        // �ȴ� 5 ��
                        std::this_thread::sleep_for(std::chrono::seconds(20));
                        //std::this_thread::sleep_for(std::chrono::minutes(30));
                    }
                    }).detach();  // ʹ�� detach ʹ�߳��ں�̨����
            }
            else {
                spdlog::info("���׵�¼ʧ��: {}", (pRspInfo ? pRspInfo->ErrorMsg : "δ֪����"));
            }
        #endif
    }


    void OnRspOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override {
        spdlog::info("����¼��������Ӧ");
        if (pRspInfo && pRspInfo->ErrorID != 0) {
            spdlog::error("�µ�ʧ�ܣ�������: {} ������Ϣ: {}", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
        }
        else {
            spdlog::info("�µ��ɹ�!");
        }
    }


    //void OnRtnOrder(CThostFtdcOrderField* pOrder) override {
    //    spdlog::info("SPI:�յ�����/����֪ͨ");
    //    if (pOrder) {
    //        // ��ӡ��־��Ϣ
    //        spdlog::info("OnRtnOrder ������:");
    //        spdlog::info("��Լ����: {}", pOrder->InstrumentID);
    //        spdlog::info("�����۸�����: {}", pOrder->OrderPriceType);
    //        spdlog::info("��������: {}", pOrder->Direction);
    //        spdlog::info("�۸�: {}", pOrder->LimitPrice);
    //        spdlog::info("����: {}", pOrder->VolumeTotalOriginal);
    //        spdlog::info("��Ч������: {}", pOrder->TimeCondition);
    //        spdlog::info("��ɽ�����: {}", pOrder->VolumeTraded);
    //        spdlog::info("ʣ������: {}", pOrder->VolumeTotal);

    //        // ����״̬Ϊ a ��ʾ��δ֪
    //        // ����״̬ 0 ȫ���ɽ���1 ���ֳɽ��ڶ��У�2 δ�ɽ����ڶ��У�3 δ�ɽ��ڶ��У�4 δ�ɽ����ڶ���
    //        spdlog::info("����״̬: {}", pOrder->OrderStatus); // ��Ӷ���״̬�����
    //        spdlog::info("----------------------------");
    //    }
    //    else {
    //        spdlog::info("pOrder Ϊ��");
    //    }

    //}

    void OnRtnOrder(CThostFtdcOrderField* pOrder) override {
        if (pOrder) {
            // ��鶩��״̬
            if (pOrder->OrderStatus == THOST_FTDC_OST_PartTradedNotQueueing) {
                // ���ɲ�����״̬
                std::cout << "�������ɲ������ѳɽ�������" << pOrder->VolumeTraded
                    << "��δ�ɽ�������" << pOrder->VolumeTotal << std::endl;

                // ������Ҫ�����������ύδ�ɽ��Ĳ���
                if (pOrder->VolumeTotal > 0) {
                    PlaceMarketOrder(pOrder->Direction);
                }
            }
            else if (pOrder->OrderStatus == THOST_FTDC_OST_AllTraded) {
                std::cout << "����ȫ���ɽ���" << std::endl;
            }
            else if (pOrder->OrderStatus == THOST_FTDC_OST_Canceled) {
                std::cout << "�����ѳ�����" << std::endl;
            }
        }
    }


    //void OnRspQryOrder(CThostFtdcOrderField* pOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override {
    //    spdlog::info("SPI:�յ���ѯ�ҵ��������յ� RspQryOrder ��ִ�У�");
    //    if (pOrder) {
    //        // ��ӡ��־��Ϣ
    //        spdlog::info("OnRtnOrder ������:");
    //        spdlog::info("InstrumentID: {}", pOrder->InstrumentID);
    //        spdlog::info("�����۸�����: {}", pOrder->OrderPriceType);
    //        spdlog::info("��������: {}", pOrder->Direction);
    //        spdlog::info("�۸�: {}", pOrder->LimitPrice);
    //        spdlog::info("����: {}", pOrder->VolumeTotalOriginal);
    //        spdlog::info("��Ч������: {}", pOrder->TimeCondition);
    //        spdlog::info("��ɽ�����: {}", pOrder->VolumeTraded);
    //        spdlog::info("ʣ������: {}", pOrder->VolumeTotal);
    //        spdlog::info("ί��ʱ��: {}", pOrder->InsertTime);
    //        spdlog::info("OrderRef: {}", pOrder->OrderRef);
    //        spdlog::info("OrderSysID: {}", pOrder->OrderSysID);
    //        spdlog::info("ExchangeID: {}", pOrder->ExchangeID);

    //        // ����״̬Ϊ a ��ʾ��δ֪
    //        // ����״̬ 0 ȫ���ɽ���1 ���ֳɽ��ڶ��У�2 δ�ɽ����ڶ��У�3 δ�ɽ��ڶ��У�4 δ�ɽ����ڶ���
    //        spdlog::info("����״̬: {}", pOrder->OrderStatus); // ��Ӷ���״̬�����
    //        spdlog::info("----------------------------");
    //    }
    //}

    void OnRspQryOrder(CThostFtdcOrderField* pOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override {
        if (pOrder && pOrder->OrderStatus == THOST_FTDC_OST_NoTradeQueueing) {
            // Order is pending (open), so attempt to cancel it
            spdlog::info("��⵽�ҵ�����Լ: {}, ��������: {}", pOrder->InstrumentID, pOrder->OrderRef);
            CancelOrder(pOrder);
        }

        if (bIsLast) {
            spdlog::info("�ҵ���ѯ���");
        }
    }

    void CancelOrder(CThostFtdcOrderField* pOrder) {
        CThostFtdcInputOrderActionField action = { 0 };
        strncpy_s(action.BrokerID, pOrder->BrokerID, sizeof(action.BrokerID) - 1);
        strncpy_s(action.InvestorID, pOrder->InvestorID, sizeof(action.InvestorID) - 1);
        action.OrderActionRef = 0;
        action.RequestID = nRequestID++;
        action.FrontID = pOrder->FrontID;
        action.SessionID = pOrder->SessionID;
        strncpy_s(action.OrderRef, pOrder->OrderRef, sizeof(action.OrderRef) - 1);
        strncpy_s(action.ExchangeID, pOrder->ExchangeID, sizeof(action.ExchangeID) - 1);
        strncpy_s(action.InstrumentID, pOrder->InstrumentID, sizeof(action.InstrumentID) - 1);
        action.ActionFlag = THOST_FTDC_AF_Delete;

        int ret = m_pTraderApi->ReqOrderAction(&action, nRequestID++);
        if (ret == 0) {
            spdlog::info("���������ѷ���, ��Լ: {}, ��������: {}", pOrder->InstrumentID, pOrder->OrderRef);
        }
        else {
            spdlog::error("��������ʧ�ܣ�������: {}", ret);
        }
    }




    void OnErrRtnOrderAction(CThostFtdcOrderActionField* pOrderAction, CThostFtdcRspInfoField* pRspInfo) override {
        spdlog::info("SPI:��������������Ӧ����ִ��ReqOrderAction�����ֶ���д����֮���CTP������ͨ���˽ӿڷ���");
    };
    void OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField* pInvestorPositionDetail, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override {
		spdlog::info("SPI:�յ���ѯ�ֲ���ϸ�������յ� OnRspQryInvestorPositionDetail ��ִ�У�");

        // �����Ӧ��Ϣ
        if (pRspInfo && pRspInfo->ErrorID != 0) {
            spdlog::error("��ѯ�ֲ���ϸʧ�ܣ��������: {}��������Ϣ: {}", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
            return;
        }

        // ��鷵�صĳֲ���ϸ�Ƿ���Ч
        if (pInvestorPositionDetail) {
            spdlog::info("Ͷ���ֲ߳���ϸ:");
            spdlog::info("���͹�˾����: {}", pInvestorPositionDetail->BrokerID);
            spdlog::info("Ͷ���ߴ���: {}", pInvestorPositionDetail->InvestorID);
            spdlog::info("��Լ����: {}", pInvestorPositionDetail->InstrumentID);
            spdlog::info("����: {}", (pInvestorPositionDetail->Direction == THOST_FTDC_D_Buy ? "��" : "��"));
            spdlog::info("����: {}", pInvestorPositionDetail->Volume);
            spdlog::info("���ּ�: {}", pInvestorPositionDetail->OpenPrice);
            spdlog::info("��������: {}", pInvestorPositionDetail->OpenDate);
            spdlog::info("�����: {}", pInvestorPositionDetail->SettlementPrice);
            spdlog::info("�ֲ�ӯ��: {}", pInvestorPositionDetail->PositionProfitByTrade);
            spdlog::info("----------------------------");
        }

        // ��������һ�����أ���������߼�
        if (bIsLast) {
            spdlog::info("���гֲ���ϸ��ѯ��ɡ�");
        }
    
    }
    void OnRspOrderAction(CThostFtdcInputOrderActionField* pOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override {
		spdlog::info("����������Ӧ");
        if (pRspInfo && pRspInfo->ErrorID != 0) {
            spdlog::error("����ʧ�ܣ�������: {}��������Ϣ: {}", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
        }
        else {
            spdlog::info("�����ɹ�������ID: {}", nRequestID);
        }
    }
    void ClosePosition(CThostFtdcInvestorPositionField* pPosition) {
        CThostFtdcInputOrderField ord = { 0 };
        strcpy(ord.BrokerID, "RohonDemo");                // Replace with your actual BrokerID
        strcpy(ord.InvestorID, "ylzccs01");             // Replace with your actual InvestorID
        strcpy(ord.ExchangeID, pPosition->ExchangeID); // Use ExchangeID from position data
        strcpy(ord.InstrumentID, pPosition->InstrumentID); // InstrumentID (e.g., "IH2412")

        ord.OrderPriceType = THOST_FTDC_OPT_AnyPrice;    // Market order for immediate execution
        ord.CombOffsetFlag[0] = THOST_FTDC_OF_Close;     // Close position (generic close, adjust as needed)
        ord.CombHedgeFlag[0] = pPosition->HedgeFlag;    // Same hedge flag as the position
        ord.VolumeTotalOriginal = pPosition->Position;  // Close the entire position
        ord.TimeCondition = THOST_FTDC_TC_IOC;          // Immediate or Cancel
        ord.VolumeCondition = THOST_FTDC_VC_AV;         // Any volume
        ord.MinVolume = 1;
        ord.ContingentCondition = THOST_FTDC_CC_Immediately;
        ord.StopPrice = 0;
        ord.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
        ord.IsAutoSuspend = 0;
        ord.UserForceClose = 0;

        // Set the order direction based on position direction
        if (pPosition->PosiDirection == THOST_FTDC_PD_Long) {
            ord.Direction = THOST_FTDC_D_Sell;   // Sell to close long positions
        }
        else if (pPosition->PosiDirection == THOST_FTDC_PD_Short) {
            ord.Direction = THOST_FTDC_D_Buy;    // Buy to close short positions
        }

        int ret = m_pTraderApi->ReqOrderInsert(&ord, nRequestID++);
        if (ret != 0) {
            std::cerr << "Failed to submit close order: " << ret << std::endl;
        }
        else {
            std::cout << "Close order submitted for " << pPosition->InstrumentID
                << " with volume " << pPosition->Position
                << " and direction " << (ord.Direction == THOST_FTDC_D_Sell ? "Sell" : "Buy")
                << std::endl;
        }
    }

private:
    CThostFtdcTraderApi* m_pTraderApi;
    int nRequestID = 0;
    TThostFtdcOrderRefType OrderRef; // 
    char lastDirection = 0;  // ���ڼ�¼�ϴε��µ�����0 ��ʾ��ʼ״̬�޷���

    void QueryOrders() {
        CThostFtdcQryOrderField qryOrder = { 0 };
        strncpy_s(qryOrder.BrokerID, "RohonDemo", sizeof(qryOrder.BrokerID) - 1);
        strncpy_s(qryOrder.InvestorID, "ylzccs01", sizeof(qryOrder.InvestorID) - 1);

        int ret = m_pTraderApi->ReqQryOrder(&qryOrder, nRequestID++);
        if (ret == 0) {
            spdlog::info("��ѯ�ҵ������ͳɹ�");
        }
        else {
            spdlog::error("��ѯ�ҵ�������ʧ�ܣ�������: {}", ret);
        }
    }

    void PlaceOrder(double price, int volume) {
        CThostFtdcInputOrderField ord = { 0 };
        strncpy_s(ord.BrokerID, "RohonDemo", sizeof(ord.BrokerID) - 1);
        strncpy_s(ord.InvestorID, "ylzccs01", sizeof(ord.InvestorID) - 1);
        strncpy_s(ord.ExchangeID, "SHFE", sizeof(ord.ExchangeID) - 1);
        strncpy_s(ord.InstrumentID, "IH2412", sizeof(ord.InstrumentID) - 1);
		strncpy_s(ord.UserID, "ylzccs01", sizeof(ord.UserID) - 1);
        ord.OrderPriceType = THOST_FTDC_OPT_LimitPrice;// �����۸��������޼�
        ord.Direction = THOST_FTDC_D_Buy;// ����������
        ord.CombOffsetFlag[0] = THOST_FTDC_OF_Open;// ��ƽ��־
        ord.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;// Ͷ���ױ���־��Ͷ��
        ord.LimitPrice = price; // �۸�
        ord.VolumeTotalOriginal = volume; // ������
        ord.TimeCondition = THOST_FTDC_TC_GFD;// ��Ч�����ͣ�������Ч
        ord.VolumeCondition = THOST_FTDC_VC_AV;// �ɽ������ͣ���������
		ord.MinVolume = 1;// ��С�ɽ�����1
		ord.ContingentCondition = THOST_FTDC_CC_Immediately;// ����������
		ord.StopPrice = 0;// ֹ���
		ord.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;// ǿƽԭ�򣺷�ǿƽ
        ord.IsAutoSuspend = 0;

        int ret_Order = m_pTraderApi->ReqOrderInsert(&ord, nRequestID++);

        if (ret_Order == 0) {
            spdlog::info("�������ͳɹ�");
        }
        else {
            spdlog::error("��������ʧ�ܣ�������: {}", ret_Order);
        }
    }

    // �м۵��µ�����
    void PlaceMarketOrder(char direction) {
        CThostFtdcInputOrderField ord = { 0 };
        strncpy_s(ord.BrokerID, "RohonDemo", sizeof(ord.BrokerID) - 1);
        strncpy_s(ord.InvestorID, "ylzccs01", sizeof(ord.InvestorID) - 1);
        strncpy_s(ord.ExchangeID, "SHFE", sizeof(ord.ExchangeID) - 1);
        strncpy_s(ord.InstrumentID, "IH2412", sizeof(ord.InstrumentID) - 1);
        strncpy_s(ord.UserID, "ylzccs01", sizeof(ord.UserID) - 1);

        ord.OrderPriceType = THOST_FTDC_OPT_AnyPrice;  // �м۵�
        ord.Direction = direction;  // ����
        ord.CombOffsetFlag[0] = THOST_FTDC_OF_Open;  // ����
        ord.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;  // Ͷ��
        ord.LimitPrice = 0;  // �м۵�����Ҫ�޼ۣ�����Ϊ0
        ord.VolumeTotalOriginal = 1;  // ������100��
        ord.TimeCondition = THOST_FTDC_TC_GFD;  // ������Ч
        ord.VolumeCondition = THOST_FTDC_VC_AV;  // ��������
        ord.MinVolume = 1;  // ��С�ɽ�����1
        ord.ContingentCondition = THOST_FTDC_CC_Immediately;  // ��������������
        ord.StopPrice = 0;  // �м۵�û��ֹ��ۣ�����Ϊ0

        // �ύ������API����
        int result = m_pTraderApi->ReqOrderInsert(&ord, 0);
        if (result != 0) {
            printf("�����ύʧ�ܣ�������룺%d\n", result);
        }
        else {
            printf("�м۵��ύ�ɹ���\n");
        }
    }

    //void CancelOrder() {
    //    spdlog::info("���𳷵�");
    //    CThostFtdcInputOrderActionField a = { 0 };

    //    strncpy_s(a.BrokerID, "RohonDemo", sizeof(a.BrokerID) - 1);
    //    strncpy_s(a.InvestorID, "ylzccs01", sizeof(a.InvestorID) - 1);
    //    strncpy_s(a.UserID, "ylzccs01", sizeof(a.UserID) - 1);
    //    strcpy_s(a.OrderSysID, "536875459");  //��ӦҪ��������OrderSysID
    //    strncpy_s(a.ExchangeID, "SHFE", sizeof(a.ExchangeID) - 1);
    //    strncpy_s(a.InstrumentID, "rb2501", sizeof(a.InstrumentID) - 1);
    //    a.ActionFlag = THOST_FTDC_AF_Delete;

    //    int ret_Cancel = m_pTraderApi->ReqOrderAction(&a, nRequestID++);
    //    if (ret_Cancel == 0) {
    //        std::cout << "���������ͳɹ�������ID: " << nRequestID - 1 << std::endl;
    //    } else {
    //        std::cerr << "����������ʧ�ܣ�������: " << ret_Cancel << std::endl;
    //    }

    //}

    void QueryInvestorPositionDetail() {

        // ������ѯͶ���ֲ߳���ϸ����ṹ��
        CThostFtdcQryInvestorPositionDetailField qryInvestorPositionDetail = { 0 };
        strncpy_s(qryInvestorPositionDetail.BrokerID, "RohonDemo", sizeof(qryInvestorPositionDetail.BrokerID) - 1);
        strncpy_s(qryInvestorPositionDetail.InvestorID, "ylzccs01", sizeof(qryInvestorPositionDetail.InvestorID) - 1);
        strncpy_s(qryInvestorPositionDetail.InstrumentID, "rb2501", sizeof(qryInvestorPositionDetail.InstrumentID) - 1); // ��ѡ��ָ����Լ����

        // ���Ͳ�ѯͶ���ֲ߳���ϸ����
        int ret_QueryInvestorPositionDetail = m_pTraderApi->ReqQryInvestorPositionDetail(&qryInvestorPositionDetail, nRequestID++);

        if (ret_QueryInvestorPositionDetail == 0) {
            std::cout << "��ѯͶ���ֲ߳���ϸ�����ͳɹ�" << std::endl;
        }
        else {
            std::cerr << "��ѯͶ���ֲ߳���ϸ������ʧ�ܣ�������: " << ret_QueryInvestorPositionDetail << std::endl;
        }
    }

    void QueryPositions() {
        // �����ѯͶ���ֲ֣߳���Ӧ��ӦOnRspQryInvestorPosition��CTPϵͳ���ֲ���ϸ��¼����Լ���ֲַ��򣬿������ڣ����������������Դ����������֡���֣����л��ܡ�
        CThostFtdcQryInvestorPositionField req = { 0 };
        strcpy(req.BrokerID, "RohonDemo");          // Replace with your broker ID
        strcpy(req.InvestorID, "ylzccs01");       // Replace with your investor ID

        int ret = m_pTraderApi->ReqQryInvestorPosition(&req, nRequestID++);
        if (ret != 0) {
            std::cerr << "Failed to request positions: " << ret << std::endl;
        }
    }

    void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField* pInvestorPosition,
        CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override {
        if (pInvestorPosition) {
            // Check if there's an open position (long or short)
            if (pInvestorPosition->Position > 0) {
                // Print position details
                std::cout << "InstrumentID: " << pInvestorPosition->InstrumentID << std::endl;
                std::cout << "Position: " << pInvestorPosition->Position << std::endl;
                std::cout << "Direction: "
                    << (pInvestorPosition->PosiDirection == THOST_FTDC_PD_Long ? "Long" : "Short")
                    << std::endl;

                // Close the position (handle both long and short)
                ClosePosition(pInvestorPosition);
            }
        }

        // If this is the last position, handle accordingly
        if (bIsLast) {
            std::cout << "Finished querying positions." << std::endl;
        }
    }

    void PlaceOrderAndCancel() {
        int nRequestID = 1;

        // 1. ���𱨵�����
        CThostFtdcInputOrderField order = { 0 };
        strncpy_s(order.BrokerID, "RohonDemo", sizeof(order.BrokerID) - 1);
        strncpy_s(order.InvestorID, "ylzccs01", sizeof(order.InvestorID) - 1);
        strncpy_s(order.UserID, "ylzccs01", sizeof(order.UserID) - 1);
        strncpy_s(order.InstrumentID, "rb2501", sizeof(order.InstrumentID) - 1);
        strncpy_s(order.ExchangeID, "SHFE", sizeof(order.ExchangeID) - 1);
        order.OrderPriceType = THOST_FTDC_OPT_LimitPrice; // �޼۵�
        order.Direction = THOST_FTDC_D_Buy; // ����
        order.VolumeTotalOriginal = 1; // ����
        order.LimitPrice = 3200; // �޼�

        int ret_Order = m_pTraderApi->ReqOrderInsert(&order, nRequestID++);
        if (ret_Order == 0) {
            std::cout << "���������ͳɹ�������ID: " << nRequestID - 1 << std::endl;

            // 2. ��������
            CThostFtdcInputOrderActionField action = { 0 };
            strncpy_s(action.BrokerID, "RohonDemo", sizeof(action.BrokerID) - 1);
            strncpy_s(action.InvestorID, "ylzccs01", sizeof(action.InvestorID) - 1);
            strncpy_s(action.UserID, "ylzccs01", sizeof(action.UserID) - 1);
            strncpy_s(action.OrderSysID, "��ӦҪ���ı�����OrderSysID", sizeof(action.OrderSysID) - 1); // �滻Ϊʵ�ʵı������
            strncpy_s(action.ExchangeID, "SHFE", sizeof(action.ExchangeID) - 1);
            action.ActionFlag = THOST_FTDC_AF_Delete; // ������־
            action.OrderActionRef = nRequestID++; // �����ı�����������

            int ret_Cancel = m_pTraderApi->ReqOrderAction(&action, nRequestID++);
            if (ret_Cancel == 0) {
                std::cout << "���������ͳɹ�������ID: " << nRequestID - 1 << std::endl;
            }
            else {
                std::cerr << "����������ʧ�ܣ�������: " << ret_Cancel << std::endl;
            }
        }
        else {
            std::cerr << "����������ʧ�ܣ�������: " << ret_Order << std::endl;
        }
    }
};


int main() {
    spdlog::info("CTP API Version: {}", CThostFtdcTraderApi::GetApiVersion());

    spdlog::info("Helloxxx, {}!", "world");

    CThostFtdcMdApi* pMdApi = CThostFtdcMdApi::CreateFtdcMdApi();
    CThostFtdcTraderApi* pTraderApi = CThostFtdcTraderApi::CreateFtdcTraderApi();

    // SPI ע�ᵽ API 
    std::unique_ptr<CustomMdSpi> pMdSpi = std::make_unique<CustomMdSpi>(pMdApi, pTraderApi);
    pMdApi->RegisterSpi(pMdSpi.get()); spdlog::info("ע������API���!");

    std::unique_ptr<CustomTraderSpi> pTraderSpi = std::make_unique<CustomTraderSpi>(pTraderApi);
    pTraderApi->RegisterSpi(pTraderSpi.get()); spdlog::info("ע�ύ��API���!");

    // ����ǰ�õ�ַ
    char frontAddressMd[] = "tcp://124.160.66.120:41253";
    pMdApi->RegisterFront(frontAddressMd); spdlog::info("��������ǰ�õ�ַ���!");
    

    char frontAddressTrader[] = "tcp://210.22.96.58:18111";
    pTraderApi->RegisterFront(frontAddressTrader); spdlog::info("���ý���ǰ�õ�ַ���!");

    // ��ʼ��API
    pMdApi->Init(); pTraderApi->Init();

    spdlog::info("......��ʼ������......");

    // �ȴ�API�߳̽���
    pMdApi->Join();
    pTraderApi->Join(); // �ȴ�����API�߳̽���


    spdlog::info("......API �߳̽���......");

    pMdApi->Release();
    pTraderApi->Release(); // �ͷŽ���API��Դ

    return 0;
}