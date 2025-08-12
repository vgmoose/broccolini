/* misc utils that are related to UI stuff, but didn't make it into chesto */
#include "./UIUtils.hpp"
#include "../libs/chesto/src/ImageElement.hpp"
#include "./Utils.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

ImageElement* createThemedIcon(std::string path, bool isLightTheme)
{
	std::string iconPath = RAMFS "res/icons/" + path + (isLightTheme ? "-light" : "") + ".svg";
	return new ImageElement(iconPath);
}

// takes any number of arguments to swap into the page template
std::string load_special_page(std::string pageName, ...)
{
	// load one of the stock "special" pages from RAMFS
	std::string specialPath = RAMFS "res/pages/" + pageName + ".html";
	std::ifstream t(specialPath);
	std::stringstream buffer;
	buffer << t.rdbuf();
	std::string ret = buffer.str();

	// replace ret {1} with the first argument, {2} with the second, etc.
	va_list args;
	va_start(args, pageName);
	for (int x = 0; x < 10; x++)
	{
		// up to 10 arguments supported to replace
		std::string replaceStr = "{" + std::to_string(x + 1) + "}";
		std::cout << "The X is " << x << " and the replaceStr is " << replaceStr
				  << std::endl;
		if (ret.find(replaceStr) == std::string::npos)
		{
			// slower, but more cautious (make sure tha replaceStr (eg. {1}) is
			// present) (apparently va_arg is hard to find the end of)
			break;
		}
		std::string replaceWith = va_arg(args, char*);
		ret = myReplace(ret, replaceStr, replaceWith);
	}
	va_end(args);
	return ret;
}