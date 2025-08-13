#ifndef ALERTMANAGER_HPP
#define ALERTMANAGER_HPP

#include <string>
#include <functional>
#include <chrono>
#include <queue>

class WebView;

struct AlertRequest {
    std::string message;
    std::function<void(bool)> callback; // Called when alert is dismissed
    std::chrono::steady_clock::time_point requestTime;
};

class AlertManager {
public:
    AlertManager(WebView* webView);
    
    // Shows an alert and calls callback when dismissed
    // Returns false if alert was blocked due to rate limiting
    bool showAlert(const std::string& message, std::function<void(bool)> callback = nullptr);
    
    // Process pending alerts and handle rate limiting
    void update();
    
    // Reset alert blocking (e.g., when navigating to new page)
    void reset();
    
    // Check if alerts are currently blocked
    bool areAlertsBlocked() const { return alertsBlocked; }
    
private:
    WebView* webView;
    
    // Rate limiting
    bool alertsBlocked = false;
    int alertCount = 0;
    std::chrono::steady_clock::time_point lastAlertTime;
    static const int MAX_ALERTS_PER_SECOND = 3;
    static const int MAX_CONSECUTIVE_ALERTS = 10;
    
    // Alert queue for managing multiple alerts
    std::queue<AlertRequest> alertQueue;
    bool isShowingAlert = false;
    
    void showNextAlert();
    void onAlertDismissed();
};

#endif // ALERTMANAGER_HPP
