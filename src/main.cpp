#include <SFML/Graphics.hpp>
#include <iostream>
int main()
{
	sf::RenderWindow window( sf::VideoMode( { 200, 200 } ), "Input Box" );
	std::cout << "Rahh\n";
	sf::String input;
	sf::Text text;
	while ( window.isOpen() )
	{
		while ( const std::optional event = window.pollEvent() )
		{
			if (event.type == sf::Event::TextEntered) {
				input += event.text.unicode;
				text.setString(input);
			}

			if ( event->is<sf::Event::Closed>() )
				window.close();
		}

		window.draw(text);

		window.clear();
		window.display();
	}
}
