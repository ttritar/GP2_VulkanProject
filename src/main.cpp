#include "core/Application.h"
#include <iostream>

int main() 
{
    Application app;

    try 
    {
        app.Run();
    }
    catch (const std::exception& e) 
    {
        std::cerr << e.what() << std::endl;
		std::cin.get();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
