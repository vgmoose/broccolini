#include "JSEngine.hpp"
#include <iostream>
#include <memory>
#include <cassert>

void testEngineCreation()
{
	std::cout << "Testing engine creation..." << std::endl;
	
	// Create an engine using the factory method
	auto engine = JSEngine::create(nullptr);  // nullptr for WebView since this is just a test
	
	assert(engine != nullptr);
	std::cout << "Engine created successfully" << std::endl;
	
	// Test JSON parsing
	bool result = engine->parseJSON(R"({"hello": "world"})");
	assert(result == true);
	std::cout << "JSON parsing test passed" << std::endl;
	
	// Test script execution  
	bool scriptResult = engine->executeScript("var x = 5; var y = 10;");
	assert(scriptResult == true);
	std::cout << "Script execution test passed" << std::endl;
	
	// Test global value setting/getting
	engine->setGlobalString("testString", "Hello World");
	std::string retrieved = engine->getGlobalString("testString");
	assert(retrieved == "Hello World");
	std::cout << "Global string test passed" << std::endl;
	
	engine->setGlobalNumber("testNumber", 42.5);
	double retrievedNumber = engine->getGlobalNumber("testNumber");
	assert(retrievedNumber == 42.5);
	std::cout << "Global number test passed" << std::endl;
}

void testCallbacks()
{
	std::cout << "Testing callback functionality..." << std::endl;
	
	auto engine = JSEngine::create(nullptr);
	assert(engine != nullptr);
	
	// Register a simple callback function
	bool callbackCalled = false;
	engine->registerGlobalFunction("testCallback", [&callbackCalled]() {
		callbackCalled = true;
	});
	
	// Call the function from JavaScript
	bool result = engine->executeScript("testCallback();");
	assert(result == true);
	assert(callbackCalled == true);
	std::cout << "Callback test passed" << std::endl;
}

int main()
{
	std::cout << "Running JavaScript Engine Tests" << std::endl;
	
	try {
		testEngineCreation();
		testCallbacks();
		
		std::cout << "All tests passed!" << std::endl;
		return 0;
	} catch (const std::exception& e) {
		std::cerr << "Test failed with exception: " << e.what() << std::endl;
		return 1;
	} catch (...) {
		std::cerr << "Test failed with unknown exception" << std::endl;
		return 1;
	}
}
