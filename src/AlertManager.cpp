#include "AlertManager.hpp"
#include "WebView.hpp"
#include <iostream>

AlertManager::AlertManager(WebView* webView) : webView(webView) 
{
    lastAlertTime = std::chrono::steady_clock::now();
}

bool AlertManager::showAlert(const std::string& message, std::function<void(bool)> callback)
{
	auto now = std::chrono::steady_clock::now();
	auto timeSinceLastAlert = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastAlertTime).count();
	
	// Rate limiting check
	if (timeSinceLastAlert < 1000) { // Less than 1 second since last alert
		alertCount++;
		if (alertCount > MAX_ALERTS_PER_SECOND) {
			if (!alertsBlocked) {
				std::cout << "[AlertManager] Too many alerts - blocking further alerts and interrupting JS execution" << std::endl;
				alertsBlocked = true;
				
				// NEW: Interrupt JavaScript execution
				if (webView && webView->jsEngine) {
					webView->jsEngine->setExecutionInterrupted(true);
					std::cout << "[AlertManager] JavaScript execution interrupted" << std::endl;
				}
				
				// Show one final alert to inform the user
				AlertRequest blockingAlert;
				blockingAlert.message = "This page is showing too many alerts and has been blocked.";
				blockingAlert.callback = [this](bool dismissed) {
					std::cout << "[AlertManager] Alert blocking notification dismissed" << std::endl;
					// Re-enable JavaScript execution after user acknowledges
					if (webView && webView->jsEngine) {
						webView->jsEngine->setExecutionInterrupted(false);
						std::cout << "[AlertManager] JavaScript execution re-enabled" << std::endl;
					}
				};
				blockingAlert.requestTime = now;
				
				alertQueue.push(blockingAlert);
				if (!isShowingAlert) {
					showNextAlert();
				}
			}
			
			// Block this alert and return false
			if (callback) callback(false);
			return false;
		}
	} else {
		// Reset counter if enough time has passed
		alertCount = 0;
	}
	
	// If alerts are blocked, don't show new ones
	if (alertsBlocked) {
		std::cout << "[AlertManager] Alert blocked: " << message << std::endl;
		if (callback) callback(false);
		return false;
	}
	
	lastAlertTime = now;
	
	// Create alert request
	AlertRequest request;
	request.message = message;
	request.callback = callback;
	request.requestTime = now;
	
	// Add to queue
	alertQueue.push(request);
	
	// Show immediately if no other alert is showing
	if (!isShowingAlert) {
		showNextAlert();
	}
	
	return true;
}void AlertManager::showNextAlert()
{
    if (alertQueue.empty() || isShowingAlert) {
        return;
    }
    
    AlertRequest request = alertQueue.front();
    alertQueue.pop();
    
    isShowingAlert = true;
    
    std::cout << "[AlertManager] Showing alert: " << request.message << std::endl;
    
    // Store the callback for when the alert is dismissed
    auto currentCallback = request.callback;
    
    // Set up the alert
    if (webView && webView->alert) {
        webView->alert->setText(request.message);
        webView->alert->title = "Alert";
        
        webView->alert->onConfirm = [this, currentCallback]() {
            webView->alert->hidden = true;
            isShowingAlert = false;
            
            // Call the callback
            if (currentCallback) {
                currentCallback(true);
            }
            
            // Show next alert if there is one
            showNextAlert();
        };
        
        webView->alert->show();
    }
}

void AlertManager::onAlertDismissed()
{
    isShowingAlert = false;
    showNextAlert();
}

void AlertManager::update()
{
}

void AlertManager::reset()
{
	alertsBlocked = false;
	alertCount = 0;
	
	// Clear pending alerts
	while (!alertQueue.empty()) {
		AlertRequest request = alertQueue.front();
		alertQueue.pop();
		if (request.callback) {
			request.callback(false); // Notify that alert was cancelled
		}
	}
	
	isShowingAlert = false;
	lastAlertTime = std::chrono::steady_clock::now();
	
	// Reset JavaScript execution interruption
	if (webView && webView->jsEngine) {
		webView->jsEngine->setExecutionInterrupted(false);
		std::cout << "[AlertManager] JavaScript execution re-enabled on page reset" << std::endl;
	}
	
	std::cout << "[AlertManager] Alert state reset" << std::endl;
}