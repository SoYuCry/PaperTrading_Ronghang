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
#include <nlohmann/json.hpp>  // 引入 nlohmann json 库
#include <thread>

using json = nlohmann::json;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// 链接库
#pragma comment (lib, "thostmduserapi_se.lib")
#pragma comment (lib, "thosttraderapi_se.lib")

// 模拟经纪商代码和投资者信息
TThostFtdcBrokerIDType gBrokerID = "RohonDemo"; // 请填写正确的BrokerID
TThostFtdcInvestorIDType gInvesterID = "ylzccs01"; // ylzccs01
TThostFtdcPasswordType gInvesterPassword = "888888"; // 888888

char frontAddressMd[] = "tcp://124.160.66.120:41253";
char frontAddressTrader[] = "tcp://210.22.96.58:18111"; // 假设的交易前置地址

class CustomMdSpi : public CThostFtdcMdSpi {
public:
    CustomMdSpi(CThostFtdcMdApi* pMdApi, CThostFtdcTraderApi* pTraderApi)
        : m_pMdApi(pMdApi), m_pTraderApi(pTraderApi), nRequestID(0) {}


    void OnFrontConnected() override {
        spdlog::info("行情服务器连接成功，登录中...");
        CThostFtdcReqUserLoginField reqUserLogin = { 0 };
        strncpy_s(reqUserLogin.BrokerID, gBrokerID, sizeof(reqUserLogin.BrokerID) - 1);
        strncpy_s(reqUserLogin.UserID, gInvesterID, sizeof(reqUserLogin.UserID) - 1);
        strncpy_s(reqUserLogin.Password, gInvesterPassword, sizeof(reqUserLogin.Password) - 1);
        
        int ret = m_pMdApi->ReqUserLogin(&reqUserLogin, nRequestID++);

        // Check 是否连接成功
        if (ret == 0) {
            spdlog::info("行情登录请求发送成功");
        } else {
            spdlog::error("行情登录请求发送失败，错误码: {}", ret);
        }
    }
    
    void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override {
        spdlog::info("收到行情登录响应");
        if (pRspInfo && pRspInfo->ErrorID == 0) {
            spdlog::info("行情登录成功>>>>>");
        }
        else {
            spdlog::error("行情登录失败: {}", (pRspInfo ? pRspInfo->ErrorMsg : "未知错误"));
            spdlog::error("请检查用户名、密码和经纪商代码是否正确。");
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

        // 客户端认证请求
        CThostFtdcReqAuthenticateField reqAuthenticate = { 0 };
        strncpy_s(reqAuthenticate.BrokerID, gBrokerID, sizeof(reqAuthenticate.BrokerID) - 1);
        strncpy_s(reqAuthenticate.UserID, gInvesterID, sizeof(reqAuthenticate.UserID) - 1);
        strncpy_s(reqAuthenticate.UserProductInfo, "YourProductInfo", sizeof(reqAuthenticate.UserProductInfo) - 1);
        strncpy_s(reqAuthenticate.AuthCode, "VHGgJNmQdEna2w4h", sizeof(reqAuthenticate.AuthCode) - 1);
        strncpy_s(reqAuthenticate.AppID, "liuc_client_0.1.0", sizeof(reqAuthenticate.AppID) - 1);

        int ret_Authenticate = m_pTraderApi->ReqAuthenticate(&reqAuthenticate, nRequestID++);
        if (ret_Authenticate == 0) {
            spdlog::info("客户端认证请求发送成功");
        }
        else {
            spdlog::error("客户端认证请求发送失败，错误码: {}", ret_Authenticate);
        }

        spdlog::info("交易服务器连接成功，登录中...");
        CThostFtdcReqUserLoginField reqUserLogin = { 0 };
        strncpy_s(reqUserLogin.BrokerID, gBrokerID, sizeof(reqUserLogin.BrokerID) - 1);
        strncpy_s(reqUserLogin.UserID, gInvesterID, sizeof(reqUserLogin.UserID) - 1);
        strncpy_s(reqUserLogin.Password, gInvesterPassword, sizeof(reqUserLogin.Password) - 1);

        int ret_UserLogin = m_pTraderApi->ReqUserLogin(&reqUserLogin, nRequestID++);

        // Check 是否连接成功
        if (ret_UserLogin == 0) {
            spdlog::info("交易登录请求发送成功");
        } else {
            spdlog::error("交易登录请求发送失败，错误码: {}", ret_UserLogin);
        }
    }
    void OnFrontDisconnected(int nReason) override {
        spdlog::info("交易服务器连接断开，原因: {}", nReason);
        isLoggedIn = false; // 更新登录状态
    }

    void OnRspAuthenticate(CThostFtdcRspAuthenticateField* pRspAuthenticateField, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override {
        spdlog::info("收到客户端认证响应");
        if (pRspInfo && pRspInfo->ErrorID == 0) {
            spdlog::info("客户端认证成功>>>>>");
        }
        else {
            spdlog::info("客户端认证失败: {}", (pRspInfo ? pRspInfo->ErrorMsg : "未知错误"));
        }
    
    };
private:
    bool isLoggedIn = false; // 添加登录状态标志
    #define USE_ALTERNATE_LOGIN_HANDLER


    void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override {
        spdlog::info("收到交易登录响应");

        #if defined(USE_ALTERNATE_LOGIN_HANDLER)
            // Alternate login handler code
            //spdlog::info("交易登录失败: {}", (pRspInfo ? pRspInfo->ErrorMsg : "未知错误"));
            char currentDirection = THOST_FTDC_D_Buy;
            //PlaceMarketOrder(currentDirection);
            //QueryPositions();
            PlaceOrder(3200,2);
            QueryOrders();

            
        #else
            // Original version of the function (default case)

            if (pRspInfo && pRspInfo->ErrorID == 0) {
                spdlog::info("交易登录成功，开始每分钟获取交易信号并下单");

                // 启动一个新的线程来每分钟检查交易信号
                std::thread([this]() {
                    while (true) {
                        CURL* curl;
                        CURLcode res;
                        std::string readBuffer;

                        // 初始化 libcurl 库
                        curl_global_init(CURL_GLOBAL_DEFAULT);
                        curl = curl_easy_init();

                        if (curl) {

                            // 模型的 URL
                            //curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.4.235:5000/trade");

                            // 测试的 URL ：http://127.0.0.1:5001
                            curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:5001/data");

                            // 设置回调函数来处理返回数据
                            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

                            // 执行请求
                            res = curl_easy_perform(curl);

                            // 检查请求是否成功
                            if (res != CURLE_OK) {
                                std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
                            }
                            else {
                                //std::cout << "Response Data: " << readBuffer << std::endl;

                                // 解析 JSON 数据
                                try {
                                    json j = json::parse(readBuffer);
                                    std::string status = j["status"];
								    if (status != "success") {
									    spdlog::info("无效的响应/收盘时间: {}", status);
									    continue;
								    }

                                    json predictions = j["predictions"];
                                    std::string firstKeyStr = "pred_30_8_20";
                                    bool firstValue = predictions[firstKeyStr];


                                    // 打印 firstValue
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
									    spdlog::info("有问题，firstValue 既不是 True 也不是 False}");
									    continue;
								    }

                                    // 检查当前信号方向是否需要下单
                                    if (lastDirection == 0) {
                                        // 如果没有持仓，直接根据当前信号方向下单
                                        std::cout << "无持仓：下一个" << (firstValue == true ? "买单" : "卖单") << std::endl;
                                        PlaceMarketOrder(currentDirection);
                                        lastDirection = currentDirection;  // 更新持仓方向
                                    }
                                    else if (lastDirection != currentDirection) {
                                        // 如果有持仓并且方向与当前信号相反，则平仓
                                        std::cout << "信号反方向：查询仓位并平仓" << std::endl;
                                        QueryPositions();

                                        // 平仓后下一单一样的
                                        std::cout << "下一个" << (firstValue == true ? "买单" : "卖单") << std::endl;
                                        PlaceMarketOrder(lastDirection == THOST_FTDC_D_Buy ? THOST_FTDC_D_Sell : THOST_FTDC_D_Buy);
                                        lastDirection = currentDirection;  // 平仓后将持仓方向重置为无
                                    }
                                    else if (lastDirection == currentDirection) {
                                        std::cout << "信号相同：继续持仓" << std::endl;
                                    }

                                }
                                catch (json::parse_error& e) {
                                    spdlog::error("JSON 解析错误: {}", e.what());
                                }
                            }

                            // 清理
                            curl_easy_cleanup(curl);
                        }

                        // 等待 5 秒
                        std::this_thread::sleep_for(std::chrono::seconds(20));
                        //std::this_thread::sleep_for(std::chrono::minutes(30));
                    }
                    }).detach();  // 使用 detach 使线程在后台运行
            }
            else {
                spdlog::info("交易登录失败: {}", (pRspInfo ? pRspInfo->ErrorMsg : "未知错误"));
            }
        #endif
    }


    void OnRspOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override {
        spdlog::info("报单录入请求响应");
        if (pRspInfo && pRspInfo->ErrorID != 0) {
            spdlog::error("下单失败，错误码: {} 错误信息: {}", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
        }
        else {
            spdlog::info("下单成功!");
        }
    }


    //void OnRtnOrder(CThostFtdcOrderField* pOrder) override {
    //    spdlog::info("SPI:收到报单/撤单通知");
    //    if (pOrder) {
    //        // 打印日志信息
    //        spdlog::info("OnRtnOrder 被调用:");
    //        spdlog::info("合约代码: {}", pOrder->InstrumentID);
    //        spdlog::info("报单价格条件: {}", pOrder->OrderPriceType);
    //        spdlog::info("买卖方向: {}", pOrder->Direction);
    //        spdlog::info("价格: {}", pOrder->LimitPrice);
    //        spdlog::info("数量: {}", pOrder->VolumeTotalOriginal);
    //        spdlog::info("有效期类型: {}", pOrder->TimeCondition);
    //        spdlog::info("今成交数量: {}", pOrder->VolumeTraded);
    //        spdlog::info("剩余数量: {}", pOrder->VolumeTotal);

    //        // 订单状态为 a 表示，未知
    //        // 订单状态 0 全部成交，1 部分成交在队列，2 未成交不在队列，3 未成交在队列，4 未成交不在队列
    //        spdlog::info("订单状态: {}", pOrder->OrderStatus); // 添加订单状态的输出
    //        spdlog::info("----------------------------");
    //    }
    //    else {
    //        spdlog::info("pOrder 为空");
    //    }

    //}

    void OnRtnOrder(CThostFtdcOrderField* pOrder) override {
        if (pOrder) {
            // 检查订单状态
            if (pOrder->OrderStatus == THOST_FTDC_OST_PartTradedNotQueueing) {
                // 部成部撤的状态
                std::cout << "订单部成部撤，已成交数量：" << pOrder->VolumeTraded
                    << "，未成交数量：" << pOrder->VolumeTotal << std::endl;

                // 根据需要，可以重新提交未成交的部分
                if (pOrder->VolumeTotal > 0) {
                    PlaceMarketOrder(pOrder->Direction);
                }
            }
            else if (pOrder->OrderStatus == THOST_FTDC_OST_AllTraded) {
                std::cout << "订单全部成交。" << std::endl;
            }
            else if (pOrder->OrderStatus == THOST_FTDC_OST_Canceled) {
                std::cout << "订单已撤销。" << std::endl;
            }
        }
    }


    //void OnRspQryOrder(CThostFtdcOrderField* pOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override {
    //    spdlog::info("SPI:收到查询挂单的请求（收到 RspQryOrder 的执行）");
    //    if (pOrder) {
    //        // 打印日志信息
    //        spdlog::info("OnRtnOrder 被调用:");
    //        spdlog::info("InstrumentID: {}", pOrder->InstrumentID);
    //        spdlog::info("报单价格条件: {}", pOrder->OrderPriceType);
    //        spdlog::info("买卖方向: {}", pOrder->Direction);
    //        spdlog::info("价格: {}", pOrder->LimitPrice);
    //        spdlog::info("数量: {}", pOrder->VolumeTotalOriginal);
    //        spdlog::info("有效期类型: {}", pOrder->TimeCondition);
    //        spdlog::info("今成交数量: {}", pOrder->VolumeTraded);
    //        spdlog::info("剩余数量: {}", pOrder->VolumeTotal);
    //        spdlog::info("委托时间: {}", pOrder->InsertTime);
    //        spdlog::info("OrderRef: {}", pOrder->OrderRef);
    //        spdlog::info("OrderSysID: {}", pOrder->OrderSysID);
    //        spdlog::info("ExchangeID: {}", pOrder->ExchangeID);

    //        // 订单状态为 a 表示，未知
    //        // 订单状态 0 全部成交，1 部分成交在队列，2 未成交不在队列，3 未成交在队列，4 未成交不在队列
    //        spdlog::info("订单状态: {}", pOrder->OrderStatus); // 添加订单状态的输出
    //        spdlog::info("----------------------------");
    //    }
    //}

    void OnRspQryOrder(CThostFtdcOrderField* pOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override {
        if (pOrder && pOrder->OrderStatus == THOST_FTDC_OST_NoTradeQueueing) {
            // Order is pending (open), so attempt to cancel it
            spdlog::info("检测到挂单，合约: {}, 报单引用: {}", pOrder->InstrumentID, pOrder->OrderRef);
            CancelOrder(pOrder);
        }

        if (bIsLast) {
            spdlog::info("挂单查询完成");
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
            spdlog::info("撤单请求已发送, 合约: {}, 报单引用: {}", pOrder->InstrumentID, pOrder->OrderRef);
        }
        else {
            spdlog::error("撤单请求失败，错误码: {}", ret);
        }
    }




    void OnErrRtnOrderAction(CThostFtdcOrderActionField* pOrderAction, CThostFtdcRspInfoField* pRspInfo) override {
        spdlog::info("SPI:报单操作请求响应，当执行ReqOrderAction后有字段填写不对之类的CTP报错则通过此接口返回");
    };
    void OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField* pInvestorPositionDetail, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override {
		spdlog::info("SPI:收到查询持仓明细的请求（收到 OnRspQryInvestorPositionDetail 的执行）");

        // 检查响应信息
        if (pRspInfo && pRspInfo->ErrorID != 0) {
            spdlog::error("查询持仓明细失败，错误代码: {}，错误信息: {}", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
            return;
        }

        // 检查返回的持仓明细是否有效
        if (pInvestorPositionDetail) {
            spdlog::info("投资者持仓明细:");
            spdlog::info("经纪公司代码: {}", pInvestorPositionDetail->BrokerID);
            spdlog::info("投资者代码: {}", pInvestorPositionDetail->InvestorID);
            spdlog::info("合约代码: {}", pInvestorPositionDetail->InstrumentID);
            spdlog::info("方向: {}", (pInvestorPositionDetail->Direction == THOST_FTDC_D_Buy ? "买" : "卖"));
            spdlog::info("数量: {}", pInvestorPositionDetail->Volume);
            spdlog::info("开仓价: {}", pInvestorPositionDetail->OpenPrice);
            spdlog::info("开仓日期: {}", pInvestorPositionDetail->OpenDate);
            spdlog::info("结算价: {}", pInvestorPositionDetail->SettlementPrice);
            spdlog::info("持仓盈亏: {}", pInvestorPositionDetail->PositionProfitByTrade);
            spdlog::info("----------------------------");
        }

        // 如果是最后一条返回，处理结束逻辑
        if (bIsLast) {
            spdlog::info("所有持仓明细查询完成。");
        }
    
    }
    void OnRspOrderAction(CThostFtdcInputOrderActionField* pOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override {
		spdlog::info("撤单请求响应");
        if (pRspInfo && pRspInfo->ErrorID != 0) {
            spdlog::error("撤单失败，错误码: {}，错误信息: {}", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
        }
        else {
            spdlog::info("撤单成功，请求ID: {}", nRequestID);
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
    char lastDirection = 0;  // 用于记录上次的下单方向，0 表示初始状态无方向

    void QueryOrders() {
        CThostFtdcQryOrderField qryOrder = { 0 };
        strncpy_s(qryOrder.BrokerID, "RohonDemo", sizeof(qryOrder.BrokerID) - 1);
        strncpy_s(qryOrder.InvestorID, "ylzccs01", sizeof(qryOrder.InvestorID) - 1);

        int ret = m_pTraderApi->ReqQryOrder(&qryOrder, nRequestID++);
        if (ret == 0) {
            spdlog::info("查询挂单请求发送成功");
        }
        else {
            spdlog::error("查询挂单请求发送失败，错误码: {}", ret);
        }
    }

    void PlaceOrder(double price, int volume) {
        CThostFtdcInputOrderField ord = { 0 };
        strncpy_s(ord.BrokerID, "RohonDemo", sizeof(ord.BrokerID) - 1);
        strncpy_s(ord.InvestorID, "ylzccs01", sizeof(ord.InvestorID) - 1);
        strncpy_s(ord.ExchangeID, "SHFE", sizeof(ord.ExchangeID) - 1);
        strncpy_s(ord.InstrumentID, "IH2412", sizeof(ord.InstrumentID) - 1);
		strncpy_s(ord.UserID, "ylzccs01", sizeof(ord.UserID) - 1);
        ord.OrderPriceType = THOST_FTDC_OPT_LimitPrice;// 报单价格条件：限价
        ord.Direction = THOST_FTDC_D_Buy;// 买卖方向：买
        ord.CombOffsetFlag[0] = THOST_FTDC_OF_Open;// 开平标志
        ord.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;// 投机套保标志：投机
        ord.LimitPrice = price; // 价格
        ord.VolumeTotalOriginal = volume; // 多少手
        ord.TimeCondition = THOST_FTDC_TC_GFD;// 有效期类型：当日有效
        ord.VolumeCondition = THOST_FTDC_VC_AV;// 成交量类型：任意数量
		ord.MinVolume = 1;// 最小成交量：1
		ord.ContingentCondition = THOST_FTDC_CC_Immediately;// 触发条件：
		ord.StopPrice = 0;// 止损价
		ord.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;// 强平原因：非强平
        ord.IsAutoSuspend = 0;

        int ret_Order = m_pTraderApi->ReqOrderInsert(&ord, nRequestID++);

        if (ret_Order == 0) {
            spdlog::info("报单发送成功");
        }
        else {
            spdlog::error("报单发送失败，错误码: {}", ret_Order);
        }
    }

    // 市价单下单函数
    void PlaceMarketOrder(char direction) {
        CThostFtdcInputOrderField ord = { 0 };
        strncpy_s(ord.BrokerID, "RohonDemo", sizeof(ord.BrokerID) - 1);
        strncpy_s(ord.InvestorID, "ylzccs01", sizeof(ord.InvestorID) - 1);
        strncpy_s(ord.ExchangeID, "SHFE", sizeof(ord.ExchangeID) - 1);
        strncpy_s(ord.InstrumentID, "IH2412", sizeof(ord.InstrumentID) - 1);
        strncpy_s(ord.UserID, "ylzccs01", sizeof(ord.UserID) - 1);

        ord.OrderPriceType = THOST_FTDC_OPT_AnyPrice;  // 市价单
        ord.Direction = direction;  // 买方向
        ord.CombOffsetFlag[0] = THOST_FTDC_OF_Open;  // 开仓
        ord.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;  // 投机
        ord.LimitPrice = 0;  // 市价单不需要限价，设置为0
        ord.VolumeTotalOriginal = 1;  // 数量：100手
        ord.TimeCondition = THOST_FTDC_TC_GFD;  // 当日有效
        ord.VolumeCondition = THOST_FTDC_VC_AV;  // 任意数量
        ord.MinVolume = 1;  // 最小成交量：1
        ord.ContingentCondition = THOST_FTDC_CC_Immediately;  // 触发条件：立即
        ord.StopPrice = 0;  // 市价单没有止损价，设置为0

        // 提交订单的API调用
        int result = m_pTraderApi->ReqOrderInsert(&ord, 0);
        if (result != 0) {
            printf("订单提交失败，错误代码：%d\n", result);
        }
        else {
            printf("市价单提交成功！\n");
        }
    }

    //void CancelOrder() {
    //    spdlog::info("发起撤单");
    //    CThostFtdcInputOrderActionField a = { 0 };

    //    strncpy_s(a.BrokerID, "RohonDemo", sizeof(a.BrokerID) - 1);
    //    strncpy_s(a.InvestorID, "ylzccs01", sizeof(a.InvestorID) - 1);
    //    strncpy_s(a.UserID, "ylzccs01", sizeof(a.UserID) - 1);
    //    strcpy_s(a.OrderSysID, "536875459");  //对应要撤报单的OrderSysID
    //    strncpy_s(a.ExchangeID, "SHFE", sizeof(a.ExchangeID) - 1);
    //    strncpy_s(a.InstrumentID, "rb2501", sizeof(a.InstrumentID) - 1);
    //    a.ActionFlag = THOST_FTDC_AF_Delete;

    //    int ret_Cancel = m_pTraderApi->ReqOrderAction(&a, nRequestID++);
    //    if (ret_Cancel == 0) {
    //        std::cout << "撤单请求发送成功，请求ID: " << nRequestID - 1 << std::endl;
    //    } else {
    //        std::cerr << "撤单请求发送失败，错误码: " << ret_Cancel << std::endl;
    //    }

    //}

    void QueryInvestorPositionDetail() {

        // 创建查询投资者持仓明细请求结构体
        CThostFtdcQryInvestorPositionDetailField qryInvestorPositionDetail = { 0 };
        strncpy_s(qryInvestorPositionDetail.BrokerID, "RohonDemo", sizeof(qryInvestorPositionDetail.BrokerID) - 1);
        strncpy_s(qryInvestorPositionDetail.InvestorID, "ylzccs01", sizeof(qryInvestorPositionDetail.InvestorID) - 1);
        strncpy_s(qryInvestorPositionDetail.InstrumentID, "rb2501", sizeof(qryInvestorPositionDetail.InstrumentID) - 1); // 可选：指定合约代码

        // 发送查询投资者持仓明细请求
        int ret_QueryInvestorPositionDetail = m_pTraderApi->ReqQryInvestorPositionDetail(&qryInvestorPositionDetail, nRequestID++);

        if (ret_QueryInvestorPositionDetail == 0) {
            std::cout << "查询投资者持仓明细请求发送成功" << std::endl;
        }
        else {
            std::cerr << "查询投资者持仓明细请求发送失败，错误码: " << ret_QueryInvestorPositionDetail << std::endl;
        }
    }

    void QueryPositions() {
        // 请求查询投资者持仓，对应响应OnRspQryInvestorPosition。CTP系统将持仓明细记录按合约，持仓方向，开仓日期（仅针对上期所和能源所，区分昨仓、今仓）进行汇总。
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

        // 1. 发起报单请求
        CThostFtdcInputOrderField order = { 0 };
        strncpy_s(order.BrokerID, "RohonDemo", sizeof(order.BrokerID) - 1);
        strncpy_s(order.InvestorID, "ylzccs01", sizeof(order.InvestorID) - 1);
        strncpy_s(order.UserID, "ylzccs01", sizeof(order.UserID) - 1);
        strncpy_s(order.InstrumentID, "rb2501", sizeof(order.InstrumentID) - 1);
        strncpy_s(order.ExchangeID, "SHFE", sizeof(order.ExchangeID) - 1);
        order.OrderPriceType = THOST_FTDC_OPT_LimitPrice; // 限价单
        order.Direction = THOST_FTDC_D_Buy; // 买入
        order.VolumeTotalOriginal = 1; // 数量
        order.LimitPrice = 3200; // 限价

        int ret_Order = m_pTraderApi->ReqOrderInsert(&order, nRequestID++);
        if (ret_Order == 0) {
            std::cout << "报单请求发送成功，请求ID: " << nRequestID - 1 << std::endl;

            // 2. 撤单请求
            CThostFtdcInputOrderActionField action = { 0 };
            strncpy_s(action.BrokerID, "RohonDemo", sizeof(action.BrokerID) - 1);
            strncpy_s(action.InvestorID, "ylzccs01", sizeof(action.InvestorID) - 1);
            strncpy_s(action.UserID, "ylzccs01", sizeof(action.UserID) - 1);
            strncpy_s(action.OrderSysID, "对应要撤的报单的OrderSysID", sizeof(action.OrderSysID) - 1); // 替换为实际的报单编号
            strncpy_s(action.ExchangeID, "SHFE", sizeof(action.ExchangeID) - 1);
            action.ActionFlag = THOST_FTDC_AF_Delete; // 撤单标志
            action.OrderActionRef = nRequestID++; // 递增的报单操作引用

            int ret_Cancel = m_pTraderApi->ReqOrderAction(&action, nRequestID++);
            if (ret_Cancel == 0) {
                std::cout << "撤单请求发送成功，请求ID: " << nRequestID - 1 << std::endl;
            }
            else {
                std::cerr << "撤单请求发送失败，错误码: " << ret_Cancel << std::endl;
            }
        }
        else {
            std::cerr << "报单请求发送失败，错误码: " << ret_Order << std::endl;
        }
    }
};


int main() {
    spdlog::info("CTP API Version: {}", CThostFtdcTraderApi::GetApiVersion());

    spdlog::info("Helloxxx, {}!", "world");

    CThostFtdcMdApi* pMdApi = CThostFtdcMdApi::CreateFtdcMdApi();
    CThostFtdcTraderApi* pTraderApi = CThostFtdcTraderApi::CreateFtdcTraderApi();

    // SPI 注册到 API 
    std::unique_ptr<CustomMdSpi> pMdSpi = std::make_unique<CustomMdSpi>(pMdApi, pTraderApi);
    pMdApi->RegisterSpi(pMdSpi.get()); spdlog::info("注册行情API完成!");

    std::unique_ptr<CustomTraderSpi> pTraderSpi = std::make_unique<CustomTraderSpi>(pTraderApi);
    pTraderApi->RegisterSpi(pTraderSpi.get()); spdlog::info("注册交易API完成!");

    // 设置前置地址
    char frontAddressMd[] = "tcp://124.160.66.120:41253";
    pMdApi->RegisterFront(frontAddressMd); spdlog::info("设置行情前置地址完成!");
    

    char frontAddressTrader[] = "tcp://210.22.96.58:18111";
    pTraderApi->RegisterFront(frontAddressTrader); spdlog::info("设置交易前置地址完成!");

    // 初始化API
    pMdApi->Init(); pTraderApi->Init();

    spdlog::info("......初始化结束......");

    // 等待API线程结束
    pMdApi->Join();
    pTraderApi->Join(); // 等待交易API线程结束


    spdlog::info("......API 线程结束......");

    pMdApi->Release();
    pTraderApi->Release(); // 释放交易API资源

    return 0;
}