#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include <iostream>
#include <string>

namespace
{
constexpr unsigned int kWindowWidth = 640;
constexpr unsigned int kWindowHeight = 200;

constexpr float kSearchBarWidth = 520.f;
constexpr float kSearchBarHeight = 44.f;
constexpr float kSearchBarPadding = 16.f;
constexpr float kCursorWidth = 2.f;
constexpr float kCursorBlinkSeconds = 0.5f;
constexpr const char *kFontPath = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";

bool isPrintableAscii(sf::Uint32 unicode)
{
	return unicode >= 32 && unicode < 127;
}
}

int main()
{
	sf::RenderWindow window(sf::VideoMode({kWindowWidth, kWindowHeight}), "SFML Search Bar");
	window.setFramerateLimit(60);

	sf::Font font;
	if (!font.loadFromFile(kFontPath))
	{
		std::cerr << "Failed to load font. Ensure " << kFontPath << " exists.\n";
		return 1;
	}

	std::string input;
	bool isFocused = true;

	sf::RectangleShape searchBar({kSearchBarWidth, kSearchBarHeight});
	searchBar.setFillColor(sf::Color(30, 30, 30));
	searchBar.setOutlineThickness(2.f);
	searchBar.setOutlineColor(sf::Color(90, 90, 90));
	searchBar.setPosition({(kWindowWidth - kSearchBarWidth) / 2.f, (kWindowHeight - kSearchBarHeight) / 2.f});

	sf::Text inputText;
	inputText.setFont(font);
	inputText.setCharacterSize(20);
	inputText.setFillColor(sf::Color::White);
	inputText.setPosition({searchBar.getPosition().x + kSearchBarPadding, searchBar.getPosition().y + 8.f});

	sf::Text placeholderText;
	placeholderText.setFont(font);
	placeholderText.setCharacterSize(20);
	placeholderText.setFillColor(sf::Color(160, 160, 160));
	placeholderText.setString("Search...");
	placeholderText.setPosition({searchBar.getPosition().x + kSearchBarPadding, searchBar.getPosition().y + 8.f});

	sf::RectangleShape cursor({kCursorWidth, static_cast<float>(inputText.getCharacterSize())});
	cursor.setFillColor(sf::Color::White);

	sf::Clock blinkClock;
	bool showCursor = true;

	while (window.isOpen())
	{
		while (const std::optional event = window.pollEvent())
		{
			if (event->is<sf::Event::Closed>())
			{
				window.close();
			}
			else if (const auto *mousePressed = event->getIf<sf::Event::MouseButtonPressed>())
			{
				sf::Vector2f mousePos(static_cast<float>(mousePressed->position.x),
									static_cast<float>(mousePressed->position.y));
				isFocused = searchBar.getGlobalBounds().contains(mousePos);
				searchBar.setOutlineColor(isFocused ? sf::Color(120, 180, 255) : sf::Color(90, 90, 90));
			}
			else if (const auto *textEntered = event->getIf<sf::Event::TextEntered>())
			{
				if (!isFocused)
				{
					continue;
				}

				const sf::Uint32 unicode = textEntered->unicode;
				if (unicode == 8)
				{
					if (!input.empty())
					{
						input.pop_back();
					}
				}
				else if (unicode == 13)
				{
					std::cout << "Search for: " << input << '\n';
					input.clear();
				}
				else if (isPrintableAscii(unicode))
				{
					input.push_back(static_cast<char>(unicode));
				}
			}
		}

		inputText.setString(input);
		const sf::FloatRect textBounds = inputText.getLocalBounds();
		cursor.setSize({kCursorWidth, textBounds.height});
		cursor.setPosition({inputText.getPosition().x + textBounds.width + 2.f,
							inputText.getPosition().y + 4.f});

		if (blinkClock.getElapsedTime().asSeconds() >= kCursorBlinkSeconds)
		{
			showCursor = !showCursor;
			blinkClock.restart();
		}

		window.clear(sf::Color(20, 20, 20));
		window.draw(searchBar);
		if (input.empty())
		{
			window.draw(placeholderText);
		}
		else
		{
			window.draw(inputText);
		}

		if (isFocused && showCursor)
		{
			window.draw(cursor);
		}

		window.display();
	}

	return 0;
}
