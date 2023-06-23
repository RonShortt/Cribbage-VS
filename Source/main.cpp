#include "UltraEngine.h"
#include <typeinfo>
#include <iostream>
#include <string>
using namespace UltraEngine;
#include <ctype.h>
//
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

extern void InitCardIcon();

class MyWidget
{
public:
	void Hide()
	{
		auto newpos = m_pos;
		newpos.y = 9999;
		m_widget->SetShape(newpos, m_sz);
		m_visible = false;
	}
	void Show()
	{
		m_widget->SetShape(m_pos, m_sz);
		m_visible = true;
	}
	void Enable()
	{
		m_widget->SetInteractive(true);
	}
	void Disable()
	{
		m_widget->SetInteractive(false);
	}
	void SetImage(shared_ptr<Icon> i)
	{
		m_widget->SetIcon(i);
	}
	void SetButtonText(string s)
	{
		m_widget->SetText(s);
	}
	shared_ptr<Widget> GetWidget()
	{
		return m_widget;
	}
	bool IsEnabled()
	{
		return m_enabled;
	}
	bool IsSelected()
	{
		return m_selected;
	}
	bool IsVisible()
	{
		return m_visible;
	}
	void SetSelected(bool s)
	{
		m_selected = s;
		if (s)
		{
			auto newpos = m_pos;
			newpos.y = 0;
			m_widget->SetShape(newpos, m_sz);
		}
		else
		{
			m_widget->SetShape(m_pos, m_sz);
		}
	}
	MyWidget() : m_pos({ 0, 0 }), m_sz({ 0, 0 }), m_widget(NULL), m_enabled(true), m_selected(false), m_visible(true) {}
	MyWidget(shared_ptr<Widget> cw) : m_pos(cw->GetPosition()), m_sz(cw->GetSize()), m_widget(cw), m_enabled(true), m_selected(false), m_visible(true) {}
	MyWidget(shared_ptr<Panel> cw) : m_pos(cw->GetPosition()), m_sz(cw->GetSize()), m_widget(cw), m_enabled(true), m_selected(false), m_visible(true) {}
private:
	iVec2 m_pos;
	iVec2 m_sz;
	shared_ptr<Widget> m_widget;
	bool m_enabled;
	bool m_selected;
	bool m_visible;
};

#define BILLION 1000000000.0

enum class RANK
{
	Ace = 0, Two, Three, Four, Five, Six, Seven, Eight, Nine, Ten, Jack, Queen, King
};

enum class SUIT
{
	Clubs = 0, Diamonds, Hearts, Spades
};

// Make them global

static shared_ptr<UltraEngine::Window> MainWindow;
static shared_ptr<Interface> MainUI;

static shared_ptr<Widget> MainPanel;
static shared_ptr<Widget> MsgPanel;
static shared_ptr<Widget> MsgPanelText;
static shared_ptr<Widget> ScorePanel;
static shared_ptr<Widget> HandPanel;
static shared_ptr<Widget> lblHumanScore;
static shared_ptr<Widget> lblComputerScore;
static MyWidget btnPlay;
static MyWidget lblHumanDeals;
static MyWidget lblComputerDeals;
static shared_ptr<Widget> lblCount;
static shared_ptr<Widget> CountPanel;
static shared_ptr<Widget> CountHeading;
static MyWidget btnHumanCard[6];
static MyWidget imgComputerCard[6];
static MyWidget imgPlayedCard[8];
static MyWidget imgCutCard;

static String countText;
static String msg;

const Vec4 BACKGROUND_COLOUR(0, 0.5, 0, 1);
const Vec4 FOREGROUND_COLOUR(1, 1, 1, 1);
const Vec4 BUTTON_BACKGROUND(0.8f, 0.8f, 0.8f, 1.0f);
const Vec4 BUTTON_FOREGROUND(0, 0, 0, 1);
#if __linux__
const Vec4 WAIT_COLOUR(((12 * 16) + 4) / 256.0, ((4 * 16) + 4) / 256.0, ((2 * 16) + 10) / 256.0, 1.0);
#else
const Vec4 WAIT_COLOUR(((13 * 16) + 6) / 256.0f, ((2 * 16) + 9) / 256.0f, ((2 * 16) + 11) / 256.0f, 0.6f);
#endif

//const auto LABEL_DEBUG = LABEL_BORDER;
const auto LABEL_DEBUG = (LabelStyle)0;
const int CARD_WIDTH = 175;
const int CARD_HEIGHT = 249;
const int CARD_OFFSET = 30;
const int LEFT_BORDER = 20;
const int SELECT_OFFSET = 40;
const int SCORE_Y = 20;
const int SCORE_HEIGHT = 30;
const int HAND_Y = SCORE_Y + 50;
const int HAND_HEIGHT = CARD_HEIGHT + SELECT_OFFSET;
const iVec2 COUNT_POSITION(CARD_WIDTH + (4 * CARD_OFFSET), HAND_Y + 30);
const iVec2 COUNT_SIZE(100, 80);
const int MSG_PANEL_WIDTH = 500;
const int MAIN_PANEL_WIDTH = 20 + (2 * (CARD_WIDTH + (5 * CARD_OFFSET))) + (COUNT_SIZE.y - 30) + 10;
const int SCREEN_WIDTH = MAIN_PANEL_WIDTH + MSG_PANEL_WIDTH;

#ifdef __linux__
const auto FONT_SCALE = 0.825;
#else
const auto FONT_SCALE = 1.0;
#endif

struct Card
{
	RANK rank;
	SUIT suit;
	int value;
	bool selected;
	std::string printable;
	shared_ptr<Icon> icon;
};

Card* playerHand[7];
Card* savePlayerHand[4];
Card* computerHand[7];
Card* saveComputerHand[4];
Card* theCrib[4];
Card theDeck[52];
Card* cutCard;
Card* playedCards[9];

bool playedDeals, doCrib, playedPlays, autoCut;

static uint64_t lastClickTime = 0;

// Function prototypes
int humanScore, computerScore, pointCount, selectedCardCount, playedCardsCount;
int* GetBest4Cards();
bool CheckForRun();
int CountHand(struct Card* hand[4]);
void update_points(int number);
bool check_for_computer_play();
bool check_for_human_play();
void CheckCardPlayed(bool humanPlayed);
void add_to_pcards(struct Card* c);
int CountHandAndCut(int count);
void display_message(const String& message);
void display_human_hand();
void DisplayComputerHand(bool);

shared_ptr<Icon> cardIcon[52];
shared_ptr<Icon> cardBackIcon;

bool btnPlayClicked(const Event& ev, shared_ptr<Object> extra);

bool btnHumanCardClickedCommon(int i);

bool btnHumanCard0Clicked(const Event& ev, shared_ptr<Object> extra)
{
	return btnHumanCardClickedCommon(0);
}

bool btnHumanCard1Clicked(const Event& ev, shared_ptr<Object> extra)
{
	return btnHumanCardClickedCommon(1);
}

bool btnHumanCard2Clicked(const Event& ev, shared_ptr<Object> extra)
{
	return btnHumanCardClickedCommon(2);
}

bool btnHumanCard3Clicked(const Event& ev, shared_ptr<Object> extra)
{
	return btnHumanCardClickedCommon(3);
}

bool btnHumanCard4Clicked(const Event& ev, shared_ptr<Object> extra)
{
	return btnHumanCardClickedCommon(4);
}

bool btnHumanCard5Clicked(const Event& ev, shared_ptr<Object> extra)
{
	return btnHumanCardClickedCommon(5);
}

typedef bool(*ClickFunction)(const Event& ev, shared_ptr<Object> extra);

ClickFunction btnHumanCardClicked[] = { btnHumanCard0Clicked,
	btnHumanCard1Clicked,
	btnHumanCard2Clicked,
	btnHumanCard3Clicked,
	btnHumanCard4Clicked,
	btnHumanCard5Clicked
};

int PositionAfter(shared_ptr<Widget> w)
{
	return w->GetPosition().x + w->GetSize().x;
}

void build_windows()
{
	iVec2 sz, pos;

	//Get the displays
	auto displays = GetDisplays();
	sz = displays[0]->GetSize();

	if ((sz.x < SCREEN_WIDTH) || (sz.y < 720))
	{
		Notify("You screen must be at least " + String(SCREEN_WIDTH) + " by 720");
		exit(1);
	}
	//Create window
	MainWindow = CreateWindow("Ultra Engine", (sz.x - SCREEN_WIDTH) / 2, (sz.y - 720) / 2, SCREEN_WIDTH, 720, displays[0]);

	//Create user interface
	MainUI = CreateInterface(MainWindow);
	MainUI->root->SetColor(BACKGROUND_COLOUR, WIDGETCOLOR_BACKGROUND);
	MainUI->root->SetColor(FOREGROUND_COLOUR, WIDGETCOLOR_FOREGROUND);
	sz = MainUI->root->ClientSize();
	MainPanel = CreatePanel(0, 0, MAIN_PANEL_WIDTH, sz.y, MainUI->root);
	MainPanel->SetColor(BACKGROUND_COLOUR);
	MsgPanel = CreatePanel(MAIN_PANEL_WIDTH, 0, sz.x - MAIN_PANEL_WIDTH, sz.y, MainUI->root);
	//	MsgPanel->SetColor(BACKGROUND_COLOUR);

	sz = MainPanel->GetSize();

	// Create the Scores panel
	ScorePanel = CreatePanel(0, SCORE_Y, sz.x, SCORE_HEIGHT, MainPanel);
	ScorePanel->SetColor(BACKGROUND_COLOUR);

	auto label = CreateLabel("Player:", LEFT_BORDER, 0, 80, 30, ScorePanel, LABEL_DEBUG | LABEL_LEFT | LABEL_MIDDLE);
	label->SetFontScale(FONT_SCALE * 1.8f);
	lblHumanScore = CreateLabel("123", PositionAfter(label), 0, 40, 30, ScorePanel, LABEL_DEBUG | LABEL_LEFT | LABEL_MIDDLE);
	lblHumanScore->SetFontScale(FONT_SCALE * 2);
	lblHumanDeals = CreateLabel("<-Dealer", PositionAfter(lblHumanScore), 0, 100, 30, ScorePanel, LABEL_DEBUG | LABEL_LEFT | LABEL_MIDDLE);
	lblHumanDeals.GetWidget()->SetFontScale(FONT_SCALE * 2);
	lblHumanDeals.Hide();

	label = CreateLabel("Computer:", LEFT_BORDER + CARD_WIDTH + (5 * CARD_OFFSET) + COUNT_SIZE.x, 0, 120, 30, ScorePanel, LABEL_DEBUG | LABEL_LEFT | LABEL_MIDDLE);
	label->SetFontScale(FONT_SCALE * 2);
	lblComputerScore = CreateLabel("123", PositionAfter(label), 0, 40, 30, ScorePanel, LABEL_DEBUG | LABEL_LEFT | LABEL_MIDDLE);
	lblComputerScore->SetFontScale(FONT_SCALE * 2);
	lblComputerDeals = CreateLabel("<-Dealer", PositionAfter(lblComputerScore), 0, 100, 30, ScorePanel, LABEL_DEBUG | LABEL_LEFT | LABEL_MIDDLE);
	lblComputerDeals.GetWidget()->SetFontScale(FONT_SCALE * 2);
	lblComputerDeals.Hide();

	// Create the Hands panel
	HandPanel = CreatePanel(0, HAND_Y, SCREEN_WIDTH, HAND_HEIGHT, MainPanel);
	HandPanel->SetColor(BACKGROUND_COLOUR);
	auto panel = CreatePanel(20, 0, CARD_WIDTH + (5 * CARD_OFFSET), CARD_HEIGHT + SELECT_OFFSET, HandPanel);
	panel->SetColor(BACKGROUND_COLOUR);
	for (int i = 0; i < 6; i++)
	{
		btnHumanCard[i] = MyWidget(CreateButton("", i * CARD_OFFSET, SELECT_OFFSET, CARD_WIDTH, CARD_HEIGHT, panel));
		btnHumanCard[i].SetImage(cardIcon[13 + i]);
		ListenEvent(EVENT_WIDGETACTION, btnHumanCard[i].GetWidget(), btnHumanCardClicked[i]);
	}

	sz = panel->GetSize();
	pos = panel->GetPosition();

	btnPlay = MyWidget(CreateButton("Discard to crib", pos.x + ((sz.y - 140) / 2), HAND_Y + HAND_HEIGHT + 10, 140, 30, MainPanel));
	btnPlay.GetWidget()->SetFontScale(FONT_SCALE * 1.5);
	btnPlay.GetWidget()->SetColor(BUTTON_FOREGROUND, WIDGETCOLOR_FOREGROUND);
	btnPlay.GetWidget()->SetColor(BUTTON_BACKGROUND, WIDGETCOLOR_BACKGROUND);
	ListenEvent(EVENT_WIDGETACTION, btnPlay.GetWidget(), btnPlayClicked);

	CountPanel = CreatePanel(COUNT_POSITION.x, (HAND_HEIGHT - COUNT_SIZE.y) / 2, COUNT_SIZE.x, COUNT_SIZE.y, HandPanel);
	CountPanel->SetColor(Vec4(0, 0, 0, 0), WIDGETCOLOR_BACKGROUND);
	CountHeading = CreateLabel("Count", 0, 0, COUNT_SIZE.x, COUNT_SIZE.y / 2, CountPanel, LABEL_DEBUG | LABEL_CENTER | LABEL_MIDDLE);
	CountHeading->SetFontScale(FONT_SCALE * 2.5);
	lblCount = CreateLabel("0", 0, COUNT_SIZE.y / 2, COUNT_SIZE.x, COUNT_SIZE.y / 2, CountPanel, LABEL_DEBUG | LABEL_CENTER | LABEL_MIDDLE);
	lblCount->SetFontScale(FONT_SCALE * 2.5);

	//	pos = lblCount->GetSize();
	//	sz = lblCount->GetPosition();
	auto panel1 = CreatePanel(PositionAfter(CountPanel), 0, CARD_WIDTH + (5 * CARD_OFFSET), CARD_HEIGHT + SELECT_OFFSET, HandPanel);
	panel1->SetColor(BACKGROUND_COLOUR);
	for (int i = 0; i < 6; i++)
	{
		imgComputerCard[i] = MyWidget(CreatePanel(i * CARD_OFFSET, SELECT_OFFSET, CARD_WIDTH, CARD_HEIGHT, panel1));
		imgComputerCard[i].SetImage(cardBackIcon);
		imgComputerCard[i].GetWidget()->SetColor(BACKGROUND_COLOUR);
	}

	sz = panel1->GetSize();
	pos = panel1->GetPosition();

	auto lblCutCard = CreateLabel("Cut Card", pos.x + ((sz.y - 140) / 2), HAND_Y + HAND_HEIGHT + 10, 140, 30, MainPanel, LABEL_DEBUG | LABEL_CENTER | LABEL_MIDDLE);
	lblCutCard->SetFontScale(FONT_SCALE * 2.0);
	lblCutCard->SetColor(BUTTON_BACKGROUND, WIDGETCOLOR_BACKGROUND);

	// Set up the played card panel
	auto panel2 = CreatePanel(LEFT_BORDER, btnPlay.GetWidget()->GetPosition().y + btnPlay.GetWidget()->GetSize().y + 10, CARD_WIDTH + 7 * CARD_OFFSET, CARD_HEIGHT, MainPanel);
	panel2->SetColor(BACKGROUND_COLOUR);
	for (int i = 0; i < 8; i++)
	{
		imgPlayedCard[i] = MyWidget(CreatePanel(i * CARD_OFFSET, 0, CARD_WIDTH, CARD_HEIGHT, panel2));
		imgPlayedCard[i].SetImage(cardBackIcon);
		//imgPlayedCard[i]->SetColor(BACKGROUND_COLOUR);
	}

	imgCutCard = MyWidget(CreatePanel(pos.x + ((sz.y - CARD_WIDTH) / 2), btnPlay.GetWidget()->GetPosition().y + btnPlay.GetWidget()->GetSize().y + 10, CARD_WIDTH, CARD_HEIGHT, MainPanel));
	imgCutCard.SetImage(cardBackIcon);

	// Set up the message panel
	sz = MsgPanel->GetSize();
	MsgPanelText = CreateTextArea(0, 0, MSG_PANEL_WIDTH, sz.y, MsgPanel, TEXTAREA_WORDWRAP);
	MsgPanelText->SetFontScale(FONT_SCALE * 1.5);
	MsgPanelText->SetInteractive(false);
	MsgPanelText->SetState(WIDGETSTATE_UNSELECTED);
	MsgPanelText->SetColor(BACKGROUND_COLOUR, WIDGETCOLOR_SUNKEN);
}

void WaitForMsg()
{
	for (int i = 0; i < 6; i++)
	{
		btnHumanCard[i].Disable();
	}
	btnPlay.Disable();
	MsgPanelText->SetText(MsgPanelText->GetText() + "\n" + "Click to continue");
	MsgPanelText->SetInteractive(true);
	MsgPanelText->SetColor(WAIT_COLOUR, WIDGETCOLOR_SUNKEN);
	while (true)
	{
		const Event ev = WaitEvent();
		switch (ev.id)
		{
		case EVENT_MOUSEDOWN:
			//if (ev.source == MsgPanelText)
			//{
			for (int i = 0; i < 6; i++)
			{
				btnHumanCard[i].Enable();
			}
			btnPlay.Enable();
			MsgPanelText->SetText("");
			MsgPanelText->SetInteractive(false);
			MsgPanelText->SetState(WIDGETSTATE_UNSELECTED);
			MsgPanelText->SetColor(BACKGROUND_COLOUR, WIDGETCOLOR_SUNKEN);
			return;
			//}
			break;
		case EVENT_WINDOWCLOSE:
			exit(0);
			break;
		}
	}
}

void load_deck(void)
{
	const string ranks[] = { "Ace", "2", "3", "4", "5", "6", "7", "8", "9", "10", "Jack", "Queen", "King" };
	const string suits[] = { "Clubs", "Diamonds", "Hearts", "Spades" };

	// Load up the deck information
	for (int lcv1 = 0; lcv1 < 4; lcv1++)
	{
		for (int lcv2 = 0; lcv2 < 13; lcv2++)
		{
			int cardIndex = (lcv1 * 13) + lcv2;
			theDeck[cardIndex].suit = (SUIT)lcv1;
			theDeck[cardIndex].rank = (RANK)lcv2;
			if (lcv2 >= 9)
			{
				theDeck[cardIndex].value = 10;
			}
			else
			{
				theDeck[cardIndex].value = lcv2 + 1;
			}
			theDeck[cardIndex].selected = false;
			theDeck[cardIndex].printable = "" + ranks[lcv2] + " of " + suits[lcv1];
			theDeck[cardIndex].icon = cardIcon[cardIndex];
		}
	}
}

void shuffle_deck()
{
	int randomCard;

	// for each card in the deck, pick another random card and swap them
	for (int lcv1 = 0; lcv1 < 52; lcv1++)
	{
		// select a random card
		do
		{
			randomCard = rand() % 52;
		} while (randomCard == lcv1);

		// swap the cards
		struct Card temp = theDeck[lcv1];
		theDeck[lcv1] = theDeck[randomCard];
		theDeck[randomCard] = temp;
	}
}	// end of method shuffle

// Function to open a dialog box with a message
void display_info(int messages, ...)
{
	char* message;
	String text;
	va_list valist;

	// Initialize the valist for the number of arguments
	va_start(valist, messages);

	// Loop through the messages
	for (int i = 0; i < messages; i++)
	{
		message = va_arg(valist, char*);
		if (i == 0)
		{
			text = message;
		}
		else
		{
			text.append(message);
		}
	}
	va_end(valist);
	MsgPanelText->SetText(text);
}

void updateScores()
{
	// Update the displayed scores
	lblHumanScore->SetText(WString(humanScore));
	lblComputerScore->SetText(WString(computerScore));

	// Check for a win
	if (humanScore > 120)
	{
		display_message(countText);
		display_message("\n\nYou won the game " + String(humanScore) + " to " + String(computerScore) + "\n");
		WaitForMsg();
		exit(0);
	}
	if (computerScore > 120)
	{
		display_message(countText);
		display_message("\n\nI won the game " + String(computerScore) + " to " + String(humanScore) + "\n");
		WaitForMsg();
		exit(0);
	}
}

void newGame()
{
	// Initialize the game stuff.
	humanScore = 0;
	computerScore = 0;
	updateScores();
}

void newHand()
{
	shuffle_deck();

	MsgPanelText->SetText("");

	if (playedDeals)
	{
		lblHumanDeals.Show();
		lblComputerDeals.Hide();
	}
	else
	{
		lblHumanDeals.Hide();
		lblComputerDeals.Show();
	}

	// deal the cards
	for (int i = 0; i < 6; i++)
	{
		int hCard, cCard;
		if (playedDeals)
		{
			hCard = i * 2;
			cCard = i * 2 + 1;
		}
		else
		{
			hCard = i * 2 + 1;
			cCard = i * 2;
		}
		playerHand[i] = &theDeck[hCard];
		computerHand[i] = &theDeck[cCard];
	}
	playerHand[6] = NULL;
	computerHand[6] = NULL;

	// Sort the cards
	for (int i = 0; i < 5; i++)
	{
		for (int i1 = 0; i1 < 5 - i; i1++)
		{
			if (playerHand[i1]->rank > playerHand[i1 + 1]->rank)
			{
				struct Card* tmpCard = playerHand[i1];
				playerHand[i1] = playerHand[i1 + 1];
				playerHand[i1 + 1] = tmpCard;
			}
			if (computerHand[i1]->rank > computerHand[i1 + 1]->rank)
			{
				struct Card* tmpCard = computerHand[i1];
				computerHand[i1] = computerHand[i1 + 1];
				computerHand[i1 + 1] = tmpCard;
			}
		}
	}

	// Show the hands
	display_human_hand();
	DisplayComputerHand(false);

	// Hide the played cards and cut card.
	for (int i = 0; i < 8; i++)
	{
		imgPlayedCard[i].Hide();
	}
	imgCutCard.Hide();

	// Hide the count.
	CountHeading->SetText("");
	lblCount->SetText("");

	// Set up to get cards for the crib.
	doCrib = true;
	btnPlay.GetWidget()->SetText("Discard to Crib");
	btnPlay.Disable();
	btnPlay.Hide();
}

int main(int argc, char* argv[])
{

	srand(Millisecs() % 10000);
	lastClickTime = 0;

	InitCardIcon();

	build_windows();

	// Hide some of the widgets
	btnPlay.Disable();
	btnPlay.Hide();

	// Hide the player and computer cards
	for (int i = 0; i < 6; i++)
	{
		btnHumanCard[i].Hide();
		imgComputerCard[i].Hide();
	}

	// Hide the played cards and cut card.
	for (int i = 0; i < 8; i++)
	{
		imgPlayedCard[i].Hide();
	}
	imgCutCard.Hide();

	// Create and shuffle the deck.
	load_deck();
	shuffle_deck();

	// Cut for the deal
	int humanCut, computerCut;

	bool keepLooping;
	keepLooping = true;
	while (keepLooping)
	{
		humanCut = rand() % 52;
		do
		{
			computerCut = rand() % 52;
		} while (humanCut == computerCut);

		// Check the result
		if (theDeck[humanCut].rank < theDeck[computerCut].rank)
		{
			playedDeals = true;
			display_info(5, "You cut the ", theDeck[humanCut].printable.c_str(), "\nComputer cut the ",
				theDeck[computerCut].printable.c_str(), "\nYou won the cut and will deal first");
			keepLooping = false;
		}
		else
		{
			if (theDeck[humanCut].rank > theDeck[computerCut].rank)
			{
				playedDeals = false;
				display_info(5, "You cut the ", theDeck[humanCut].printable.c_str(), "\nComputer cut the ",
					theDeck[computerCut].printable.c_str(), "\nComputer won the cut and will deal first");
				keepLooping = false;
			}
			else
			{
				display_info(5, "You cut the ", theDeck[humanCut].printable.c_str(), "\nComputer cut the ",
					theDeck[computerCut].printable.c_str(), "\nWe cut the same and will cut again");
			}
		}
	}

	newGame();
	newHand();

	while (true)
	{
		const Event ev = WaitEvent();
		switch (ev.id)
		{
		case EVENT_WINDOWCLOSE:
			return 0;
			break;
		}
	}

	return EXIT_SUCCESS;
}

void set_card_select(int card, bool flag)
{
	if (flag)
	{
		btnHumanCard[card].SetSelected(true);
		playerHand[card]->selected = true;
	}
	else
	{
		btnHumanCard[card].SetSelected(false);
		playerHand[card]->selected = false;
	}
}

bool btnHumanCardClickedCommon(int cardNumber)
{

	if (!btnHumanCard[cardNumber].IsEnabled())
	{
		return false;
	}
	int cardsSelected = 0;

	// Find the current number of cards selected
	for (int lcv1 = 0; lcv1 < 6 && playerHand[lcv1] != NULL; lcv1++)
	{
		if (playerHand[lcv1]->selected)
		{
			cardsSelected += 1;
		}
	}

	// Check for double click
	if (!doCrib)
	{
		auto currentClickTime = Millisecs();
		if (playerHand[cardNumber]->selected)
		{
			if (lastClickTime > 0)
			{
				auto timeBetweenClicks = (currentClickTime - lastClickTime);
				if (timeBetweenClicks < 400)
				{
					shared_ptr<UltraEngine::Object > MyNull;
					EmitEvent(EVENT_WIDGETACTION, btnPlay.GetWidget());
					return false;
				}
			}
		}
		else
		{
			lastClickTime = currentClickTime;
		}
	}

	// Is the card already selected?
	if (playerHand[cardNumber]->selected)
	{
		// It is already selected. Deselect it.
		set_card_select(cardNumber, false);
		cardsSelected -= 1;
	}
	else
	{
		// It is not currently selected. Make sure we don't already have 2 selected before selecting this one.
		if (doCrib)
		{
			if (cardsSelected < 2)
			{
				set_card_select(cardNumber, true);
				cardsSelected += 1;
			}
		}
		else
		{
			if (cardsSelected < 1)
			{
				set_card_select(cardNumber, true);
				cardsSelected += 1;
			}
		}
	}
	if ((doCrib && cardsSelected == 2) || ((!doCrib) && cardsSelected == 1))
	{
		btnPlay.Enable();
		btnPlay.Show();
	}
	else
	{
		btnPlay.Disable();
		btnPlay.Hide();
	}
	return false;
}

void display_human_hand()
{
	for (int i = 0; i < 6; i++)
	{
		if (playerHand[i] == NULL)
		{
			btnHumanCard[i].Hide();
		}
		else
		{
			btnHumanCard[i].SetImage(playerHand[i]->icon);
			btnHumanCard[i].Show();
			playerHand[i]->selected = false;
		}
	}

	selectedCardCount = 0;
	btnPlay.Disable();
	btnPlay.Hide();
}

void DisplayComputerHand(bool faceUp)
{
	for (int i = 0; i < 6; i++)
	{
		if (computerHand[i] == NULL)
		{
			imgComputerCard[i].Hide();
		}
		else
		{
			if (faceUp)
			{
				imgComputerCard[i].SetImage(computerHand[i]->icon);
			}
			else
			{
				imgComputerCard[i].SetImage(cardBackIcon);
			}
			imgComputerCard[i].Show();
		}
	}
}

bool btnPlayClicked(const Event& ev, shared_ptr<Object> extra)
{
	if (!btnPlay.IsEnabled())
	{
		return false;
	}
	if (doCrib)
	{
		// Processing the crib
		doCrib = false;

		// Disable the button and make the count fields visible
		btnPlay.Disable();
		btnPlay.Hide();
		btnPlay.SetButtonText("Play card");
		CountHeading->SetText("Count");
		lblCount->SetText("0");

		// Save the player's cards which he is keeping and discard to the crib
		int saveIndex = 0, cribIndex = 0;
		for (int i = 0; i < 6; i++)
		{
			if (playerHand[i]->selected)
			{
				theCrib[cribIndex++] = playerHand[i];
				set_card_select(i, false);
				playerHand[i] = NULL;
			}
			else
			{
				savePlayerHand[saveIndex++] = playerHand[i];
			}
		}
		selectedCardCount = 0;

		printf("Player (after crib 1): ");
		for (int i = 0; i < 6; i++)
		{
			if (playerHand[i] == NULL)
			{
				printf("NULL, ");
			}
			else
			{
				printf("%s, ", playerHand[i]->printable.c_str());
			}
		}
		printf("\n");
		printf("Crib: ");
		for (int i = 0; i < cribIndex; i++)
		{
			printf("%s, ", theCrib[i]->printable.c_str());
		}
		printf("\n");

		// Get the players hand ready to play and display it.
		for (int i = 0; i < 6; i++)
		{
			while (playerHand[i] == NULL && i < 4)
			{
				for (int i1 = i; i1 < 5; i1++)
				{
					playerHand[i1] = playerHand[i1 + 1];
				}
				playerHand[5] = NULL;
			}
		}

		printf("Player (after crib 2): %s, %s, %s, %s\n", playerHand[0]->printable.c_str(), playerHand[1]->printable.c_str(),
			playerHand[2]->printable.c_str(), playerHand[3]->printable.c_str());

		display_human_hand();

		if (playedDeals)
		{
			int humanCut;
			humanCut = rand() % 40;
			cutCard = &theDeck[humanCut];
			imgCutCard.SetImage(cutCard->icon);
			imgCutCard.Show();
			if (cutCard->rank == RANK::Jack)
			{
				display_message("You get 2 points for his Nobs\n");
				humanScore += 2;
			}
			updateScores();
		}
		else
		{
			cutCard = &theDeck[20];
			imgCutCard.SetImage(cutCard->icon);
			imgCutCard.Show();
			if (cutCard->rank == RANK::Jack)
			{
				display_message("Computer get 2 points for his Nobs\n");
				computerScore += 2;
			}
			updateScores();
		}

		// Get the best 4 cards from the computer hand
		int* bestChoice = GetBest4Cards();
		for (int i = 0; i < 4; i++)
		{
			saveComputerHand[i] = computerHand[*(bestChoice + i)];
		}
		theCrib[2] = NULL;
		theCrib[3] = NULL;
		for (int i = 0; i < 6; i++)
		{
			bool gotIt = false;
			for (int i1 = 0; i1 < 4 && (!gotIt); i1++)
			{
				if (i == bestChoice[i1])
				{
					gotIt = true;
				}
			}
			if (!gotIt)
			{
				if (theCrib[2] == NULL)
				{
					theCrib[2] = computerHand[i];
				}
				else
				{
					theCrib[3] = computerHand[i];
				}
			}
		}
		for (int i = 0; i < 4; i++)
		{
			computerHand[i] = saveComputerHand[i];
		}
		computerHand[4] = NULL;
		computerHand[5] = NULL;

		printf("Computer: %s, %s, %s, %s\n", computerHand[0]->printable.c_str(), computerHand[1]->printable.c_str(),
			computerHand[2]->printable.c_str(), computerHand[3]->printable.c_str());
		printf("Crib: %s, %s, %s, %s\n", theCrib[0]->printable.c_str(), theCrib[1]->printable.c_str(),
			theCrib[2]->printable.c_str(), theCrib[3]->printable.c_str());

		DisplayComputerHand(false);

		// initialize hand play.
		if (playedDeals)
		{
			int choice = rand() % 4;
			playedCards[0] = computerHand[choice];
			playedCards[1] = NULL;
			imgPlayedCard[0].SetImage(computerHand[choice]->icon);
			imgPlayedCard[0].Show();
			playedCardsCount = 1;
			pointCount = computerHand[choice]->value;
			update_points(pointCount);
			for (int i = choice; computerHand[i] != NULL; i++)
				computerHand[i] = computerHand[i + 1];
			DisplayComputerHand(false);
		}
		else
		{
			pointCount = 0;
			update_points(0);
			playedCardsCount = 0;
			playedCards[0] = NULL;
		}
		playedPlays = true;
		// btnPlay.Enabled = true;
	}
	else
	{
		// This is the main play loop
		int choice = -1;
		for (int i = 0; i < 6 && playerHand[i] != NULL; i++)
		{
			if (playerHand[i]->selected)
			{
				choice = i;
				set_card_select(i, false);
			}
		}
		// Make sure we found a selection
		if (choice == -1)
		{
			printf("No selected card found\n");
			exit(1);
		}

		// Verify that the selection will fit at 31 or less.
		if (pointCount + playerHand[choice]->value > 31)
		{
			display_message("That card goes over 31\n");
			display_human_hand();
			return false;
		}
		// Play the card
		add_to_pcards(playerHand[choice]);
		for (int i = choice; playerHand[i] != NULL; i++)
		{
			playerHand[i] = playerHand[i + 1];
		}
		display_human_hand();
		CheckCardPlayed(true);
		// Check if the computer can make a play
		if (check_for_computer_play())
		{
			while (check_for_computer_play())
			{
				for (int i = 0; computerHand[i] != NULL; i++)
				{
					struct Card* c = computerHand[i];
					if (pointCount + c->value <= 31)
					{
						add_to_pcards(c);
						for (int i1 = i; computerHand[i1] != NULL; i1++)
						{
							computerHand[i1] = computerHand[i1 + 1];
						}
						DisplayComputerHand(false);
						CheckCardPlayed(false);
						// Check if the player has a valid play
						if (check_for_human_play())
						{
							return false;
						}
						if (!check_for_computer_play())
						{
							if (pointCount == 31)
							{
								display_message("Computer gets 2 points for 31\n");
								WaitForMsg();
								computerScore += 2;
							}
							else
							{
								display_message("Computer gets 1 point for GO\n");
								WaitForMsg();

								computerScore += 1;
							}
							updateScores();
							update_points(0);
							pointCount = 0;
							playedCards[0] = NULL;
							for (int i1 = 0; i1 < 8; i1++)
							{
								if (imgPlayedCard[i1].IsVisible())
								{
									imgPlayedCard[i1].SetImage(cardBackIcon);
								}
							}
							if (check_for_human_play())
							{
								return false;
							}
						}
						else
						{
							display_message("You say GO\n");
						}
						break;
					}
				}
			}
		}
		else
		{
			// Computer can't make a play
			if (check_for_human_play())
			{
				display_message("Computer says GO\n");
				return false;
			}
			if (pointCount == 31)
			{
				display_message("You get 2 points for 31\n");
				WaitForMsg();
				humanScore += 2;
			}
			else
			{
				display_message("Computer says GO\nYou get 1 point\n");
				WaitForMsg();
				humanScore += 1;
			}
			updateScores();
			pointCount = 0;
			update_points(0);
			playedCards[0] = NULL;
			for (int i1 = 0; i1 < 8; i1++)
			{
				if (imgPlayedCard[i1].IsVisible())
				{
					imgPlayedCard[i1].SetImage(cardBackIcon);
				}
			}
			while (check_for_computer_play())
			{
				for (int i = 0; computerHand[i] != NULL; i++)
				{
					struct Card* c = computerHand[i];
					if (pointCount + c->value <= 31)
					{
						add_to_pcards(c);
						do {
							computerHand[i] = computerHand[i + 1];
							i++;
						} while (computerHand[i] != NULL);
						DisplayComputerHand(false);
						CheckCardPlayed(false);
						// Check if the player has a valid play
						if (check_for_human_play())
						{
							return false;
						}
						if (check_for_computer_play())
						{
							display_message("You say GO\n");
						}
						else
						{
							if (pointCount == 31)
							{
								display_message("Computer gets 2 points for 31\n");
								WaitForMsg();
								computerScore += 2;
							}
							else
							{
								display_message("Computer gets 1 point for GO\n");
								WaitForMsg();
								computerScore += 1;
							}
							updateScores();
							update_points(0);
							pointCount = 0;
							playedCards[0] = NULL;
							for (int i1 = 0; i1 < 8; i1++)
							{
								if (imgPlayedCard[i1].IsVisible())
								{
									imgPlayedCard[i1].SetImage(cardBackIcon);
								}
							}
						}
						break;
					}
				}
				if (check_for_human_play())
					return false;
			}
		}
		display_human_hand();
		// Check for end of hand processing
		if (computerHand[0] == NULL && playerHand[0] == NULL)
		{
			// Display all the cards
			for (int i = 0; i < 4; i++)
			{
				btnHumanCard[i].SetImage(savePlayerHand[i]->icon);
				btnHumanCard[i].Show();
				imgComputerCard[i].SetImage(saveComputerHand[i]->icon);
				imgComputerCard[i].Show();
				imgPlayedCard[i].SetImage(theCrib[i]->icon);
				imgPlayedCard[i].Show();
			}
			for (int i = 4; i < 8; i++)
				imgPlayedCard[i].Hide();

			// empty the GRtring to hold the counting messages.
			countText = "";

			if (playedDeals)
			{
				playedDeals = false;
				int i = CountHandAndCut(1);
				countText.append("\nComputer scored " + String(i) + " points\n");
				countText.append(msg);
				computerScore += i;
				updateScores();

				i = CountHandAndCut(0);
				countText.append("\nYou scored " + String(i) + " points\n");
				countText.append(msg);
				humanScore += i;
				updateScores();

				i = CountHandAndCut(2);
				countText.append("\nYou scored " + String(i) + " points in the crib\n");
				countText.append(msg);
				humanScore += i;
				updateScores();
			}
			else
			{
				playedDeals = true;
				int i = CountHandAndCut(0);
				countText.append("You scored " + String(i) + " points\n");
				countText.append(msg);
				humanScore += i;
				updateScores();

				i = CountHandAndCut(1);
				countText.append("\nComputer scored " + String(i) + " points\n");
				countText.append(msg);
				computerScore += i;
				updateScores();

				i = CountHandAndCut(2);
				countText.append("\nComputer scored " + String(i) + " points in the crib\n");
				countText.append(msg);
				computerScore += i;
				updateScores();
			}
			display_message(countText);
			WaitForMsg();
			countText = "";

			newHand();
		}
	}
	return false;
}

void update_points(int number)
{
	lblCount->SetText(String(number));
	CountHeading->SetText("Count");
}

int totalCount;
struct Card* workHand[4];

int* GetBest4Cards()
{
	struct Card* fourCards[4];
	int bestCount = 0;
	static int bestChoice[4] = { 0, 1, 2, 3 };

	for (int i1 = 0; i1 < 3; i1++)
	{
		fourCards[0] = computerHand[i1];
		for (int i2 = i1 + 1; i2 < 4; i2++)
		{
			fourCards[1] = computerHand[i2];
			for (int i3 = i2 + 1; i3 < 5; i3++)
			{
				fourCards[2] = computerHand[i3];
				for (int i4 = i3 + 1; i4 < 6; i4++)
				{
					fourCards[3] = computerHand[i4];

					int score = CountHand(fourCards);
					if (score > bestCount)
					{
						bestChoice[0] = i1;
						bestChoice[1] = i2;
						bestChoice[2] = i3;
						bestChoice[3] = i4;
						bestCount = score;
					}
				}
			}
		}
	}
	return bestChoice;
}

void g_string_append(String s1, String s2)
{
	s1.append(s2);
}

int CountHandAndCut(int count)
{
	msg = "";
	totalCount = 0;
	struct Card* workHand[5];
	for (int i = 0; i < 4; i++)
	{
		switch (count)
		{
		case 0:
			workHand[i] = savePlayerHand[i];
			break;
		case 1:
			workHand[i] = saveComputerHand[i];
			break;
		case 2:
			workHand[i] = theCrib[i];
			break;
		}
	}
	workHand[4] = cutCard;

	// Look for a flush
	if (workHand[0]->suit == workHand[1]->suit && workHand[1]->suit == workHand[2]->suit &&
		workHand[2]->suit == workHand[3]->suit)
	{
		if (cutCard->suit == workHand[0]->suit)
		{
			msg.append("5 for a flush.\n");
			totalCount += 5;
		}
		else
		{
			msg.append("4 for a flush.\n");
			totalCount += 4;
		}
	}

	// Look for His Nobs
	for (int i = 0; i < 4; i++)
	{
		if (workHand[i]->rank == RANK::Jack)
		{
			if (workHand[i]->suit == cutCard->suit)
			{
				msg.append("1 for his nobs (" + workHand[i]->printable + "\n");
				totalCount += 1;
			}
		}
	}

	// sort the cards
	for (int i = 0; i < 4; i++)
	{
		for (int i1 = 0; i1 < 4 - i; i1++)
		{
			if (workHand[i1]->rank > workHand[i1 + 1]->rank)
			{
				struct Card* temp = workHand[i1];
				workHand[i1] = workHand[i1 + 1];
				workHand[i1 + 1] = temp;
			}
		}
	}

	// Get the ranks and suits to be more efficient
	int value[5];
	int rank[5];
	for (int i = 0; i < 5; i++)
	{
		rank[i] = static_cast<int>(workHand[i]->rank);
		value[i] = workHand[i]->value;
	}

	// look for pairs
	for (int i = 0; i < 4; i++)
	{
		for (int i1 = i + 1; i1 < 5; i1++)
		{
			if (rank[i] == rank[i1])
			{
				msg.append("2 for a pair (" + workHand[i]->printable + " + " + workHand[i1]->printable + ")\n");
				totalCount += 2;
			}
		}
	}

	// Look for 2 card 15s
	for (int i = 0; i < 4; i++)
	{
		for (int i1 = i + 1; i1 < 5; i1++)
		{
			if (value[i] + value[i1] == 15)
			{
				msg.append("2 for 15 (" + workHand[i]->printable + " + " + workHand[i1]->printable + ")\n");
				totalCount += 2;
			}
		}
	}

	// Look for 3 card 15s
	for (int i = 0; i < 3; i++)
	{
		for (int i1 = i + 1; i1 < 4; i1++)
		{
			for (int i2 = i1 + 1; i2 < 5; i2++)
			{
				if (value[i] + value[i1] + value[i2] == 15)
				{
					msg.append("2 for 15 (" + workHand[i]->printable + " + " + workHand[i1]->printable + " + " +
						workHand[i2]->printable + ")\n");
					totalCount += 2;
				}
			}
		}
	}

	// Look for 4 card 15s
	for (int i = 0; i < 2; i++)
	{
		for (int i1 = i + 1; i1 < 3; i1++)
		{
			for (int i2 = i1 + 1; i2 < 4; i2++)
			{
				for (int i3 = i2 + 1; i3 < 5; i3++)
				{
					if (value[i] + value[i1] + value[i2] + value[i3] == 15)
					{
						msg.append("2 for 15 (" + workHand[i]->printable + " + " + workHand[i1]->printable + " + " +
							workHand[i2]->printable + " + " + workHand[i3]->printable + ")\n");
						totalCount += 2;
					}
				}
			}
		}
	}

	// Look for 5 card 15
	if (value[0] + value[1] + value[2] + value[3] + value[4] == 15)
	{
		msg.append("2 for 15 (" + workHand[0]->printable + " + " + workHand[1]->printable + " + " +
			workHand[2]->printable + " + " + workHand[3]->printable + " + " + workHand[4]->printable);
		totalCount += 2;
	}

	// Look for 5 card run
	bool gotRun = false;
	if (rank[0] == rank[1] - 1 && rank[1] == rank[2] - 1 && rank[2] == rank[3] - 1 && rank[3] == rank[4] - 1)
	{
		msg.append("5 for a run (" + workHand[0]->printable + " + " + workHand[1]->printable + " + " +
			workHand[2]->printable + " + " + workHand[3]->printable + " + " + workHand[4]->printable + ")\n");
		totalCount += 5;
		gotRun = true;
	}

	if (!gotRun)
	{
		// Look for 4 card runs
		if (rank[0] == rank[1] - 1 && rank[1] == rank[2] - 1 && rank[2] == rank[3] - 1)
		{
			msg.append("4 for a run (" + workHand[0]->printable + " + " + workHand[1]->printable + " + " +
				workHand[2]->printable + " + " + workHand[3]->printable + ")\n");
			totalCount += 4;
			gotRun = true;
		}
		if (rank[0] == rank[1] - 1 && rank[1] == rank[2] - 1 && rank[2] == rank[4] - 1)
		{
			msg.append("4 for a run (" + workHand[0]->printable + " + " + workHand[1]->printable + " + " +
				workHand[2]->printable + " + " + workHand[4]->printable + ")\n");
			totalCount += 4;
			gotRun = true;
		}
		if (rank[0] == rank[1] - 1 && rank[1] == rank[3] - 1 && rank[3] == rank[4] - 1)
		{
			msg.append("4 for a run (" + workHand[0]->printable + " + " + workHand[1]->printable + " + " +
				workHand[3]->printable + " + " + workHand[4]->printable + ")\n");
			totalCount += 4;
			gotRun = true;
		}
		if (rank[0] == rank[2] - 1 && rank[2] == rank[3] - 1 && rank[3] == rank[4] - 1)
		{
			msg.append("4 for a run (" + workHand[0]->printable + " + " + workHand[2]->printable + " + " +
				workHand[3]->printable + " + " + workHand[4]->printable + ")\n");
			totalCount += 4;
			gotRun = true;
		}
		if (rank[1] == rank[2] - 1 && rank[2] == rank[3] - 1 && rank[3] == rank[4] - 1)
		{
			msg.append("4 for a run (" + workHand[1]->printable + " + " + workHand[2]->printable + " + " +
				workHand[3]->printable + " + " + workHand[4]->printable + ")\n");
			totalCount += 4;
			gotRun = true;
		}
	}

	// Search for 3 card runs
	if (!gotRun)
	{
		for (int i = 0; i < 3; i++)
		{
			for (int i1 = i + 1; i1 < 4; i1++)
			{
				for (int i2 = i1 + 1; i2 < 5; i2++)
				{
					if (rank[i] == rank[i1] - 1 && rank[i1] == rank[i2] - 1)
					{
						msg.append("3 for a run (" + workHand[i]->printable + " + " + workHand[i1]->printable + " + " +
							workHand[i2]->printable + ")\n");
						totalCount += 3;
					}
				}
			}
		}
	}

	return totalCount;
}

int CountHand(struct Card* hand[4])
{
	totalCount = 0;
	for (int i = 0; i < 4; i++)
	{
		workHand[i] = hand[i];
	}

	// Look for a flush
	if (hand[0]->suit == hand[1]->suit && hand[1]->suit == hand[2]->suit && hand[2]->suit == hand[3]->suit)
	{
		totalCount += 4;
	}

	// sort the cards
	for (int i = 0; i < 3; i++)
	{
		for (int i1 = 0; i1 < 3 - i; i1++)
		{
			if (workHand[i1]->rank > workHand[i1 + 1]->rank)
			{
				struct Card* temp = workHand[i1];
				workHand[i1] = workHand[i1 + 1];
				workHand[i1 + 1] = temp;
			}
		}
	}

	// Get the ranks and suits to be more efficient
	int rank[4], value[4];
	for (int i = 0; i < 4; i++)
	{
		rank[i] = static_cast<int>(workHand[i]->rank);
		value[i] = workHand[i]->value;
	}

	// look for pairs
	for (int i = 0; i < 3; i++)
	{
		for (int i1 = i + 1; i1 < 4; i1++)
		{
			if (rank[i] == rank[i1])
			{
				totalCount += 2;
			}
		}
	}

	// Look for 2 card 15s
	for (int i = 0; i < 3; i++)
	{
		for (int i1 = i + 1; i1 < 4; i1++)
		{
			if (value[i] + value[i1] == 15)
			{
				totalCount += 2;
			}
		}
	}
	// Look for 3 card 15s
	for (int i = 0; i < 2; i++)
	{
		for (int i1 = i + 1; i1 < 3; i1++)
		{
			for (int i2 = i1 + 1; i2 < 4; i2++)
			{
				if (value[i] + value[i1] + value[i2] == 15)
				{
					totalCount += 2;
				}
			}
		}
	}
	// Look for 4 card 15s
	if (value[0] + value[1] + value[2] + value[3] == 15)
	{
		totalCount += 2;
	}
	// Look for 4 card run
	if (rank[0] == rank[1] - 1 && rank[1] == rank[2] - 1 && rank[2] == rank[3] - 1)
	{
		totalCount += 5;
	}
	else
	{
		// Look for 3 card runs
		for (int i = 0; i < 2; i++)
		{
			{			for (int i1 = i + 1; i1 < 3; i1++)
				for (int i2 = i1 + 1; i2 < 4; i2++)
				{
					if (rank[i] == rank[i1] - 1 && rank[i1] == rank[i2] - 1)
					{
						totalCount += 3;
					}
				}
			}
		}
	}

	return totalCount;
}

void CheckCardPlayed(bool humanPlayed)
{
	// Check for 15
	if (pointCount == 15)
	{
		if (humanPlayed)
		{
			humanScore += 2;
			display_message("You scored 2 points for 15\n");
		}
		else
		{
			computerScore += 2;
			display_message("Computer scored 2 points for 15\n");
		}
		updateScores();
	}

	// Count how many cards are face up
	int count = 0;
	while (playedCards[count] != NULL)
	{
		count++;
	}

	// Check for 2, 3, or 4 of a kind
	bool gotAlike = false;
	if (count >= 4)
	{
		if (playedCards[count - 1]->rank == playedCards[count - 2]->rank && playedCards[count - 2]->rank == playedCards[count - 3]->rank &&
			playedCards[count - 3]->rank == playedCards[count - 4]->rank)
		{
			if (humanPlayed)
			{
				humanScore += 12;
				display_message("You scored 12 points for 4 of a kind\n");
			}
			else
			{
				computerScore += 12;
				display_message("Computer scored 12 points for 4 of a kind\n");
			}
			updateScores();
			gotAlike = true;
		}
	}
	if ((!gotAlike) && count >= 3)
	{
		if (playedCards[count - 1]->rank == playedCards[count - 2]->rank && playedCards[count - 2]->rank == playedCards[count - 3]->rank)
		{
			if (humanPlayed)
			{
				humanScore += 6;
				display_message("You scored 6 points for 3 of a kind\n");
			}
			else
			{
				computerScore += 6;
				display_message("Computer scored 6 points for 3 of a kind\n");
			}
			updateScores();
			gotAlike = true;
		}
	}
	if ((!gotAlike) && count >= 2)
	{
		if (playedCards[count - 1]->rank == playedCards[count - 2]->rank)
		{
			if (humanPlayed)
			{
				humanScore += 2;
				display_message("You scored 2 points for a pair\n");
			}
			else
			{
				computerScore += 2;
				display_message("Computer scored 2 points for a pair\n");
			}
			updateScores();
			gotAlike = true;
		}
	}

	// Look for runs
	struct Card* c[8];

	if (count >= 3)
	{
		// Look for runs starting from the longest
		bool got_run = false;
		for (int check = count; check >= 3 && !got_run; check--)
		{
			// copy the pointers so we don't change the original order
			for (int i1 = 0; i1 < check; i1++)
			{
				c[i1] = playedCards[i1 + (count - check)];
			}
			// Sort the cards by rank
			for (int i1 = 0; i1 < check - 1; i1++)
			{
				for (int i2 = 0; i2 < check - i1 - 1; i2++)
				{
					if (c[i2]->rank > c[i2 + 1]->rank)
					{
						struct Card* hold = c[i2];
						c[i2] = c[i2 + 1];
						c[i2 + 1] = hold;
					}
				}
			}

			got_run = true;
			for (int i1 = 0; i1 < check - 1 && got_run; i1++)
			{
				if (static_cast<int>(c[i1]->rank) != static_cast<int>(c[i1 + 1]->rank) - 1)
				{
					got_run = false;
				}
			}
			if (got_run)
			{
				if (humanPlayed)
				{
					humanScore += check;
					display_message("You scored " + String(check) + " points for a run\n");
				}
				else
				{
					computerScore += check;
					display_message("Computer scored " + String(check) + " points for a run\n");
				}
				updateScores();
			}
		}
	}
}

bool CheckForRun()
{
	// copy the pointers so we don't change the original order
	struct Card* c[8];
	int length = 0;
	for (int i = 0; i < 8; i++)
	{
		if (playedCards[i] == NULL)
		{
			i = 8;
		}
		else
		{
			c[i] = playedCards[i];
			length++;
		}
	}

	// Sort the cards by rank
	for (int i = 0; i < length - 1; i++)
	{
		for (int i1 = 0; i1 < length - 1 - i; i1++)
		{
			if (c[i1]->rank > c[i1 + 1]->rank)
			{
				struct Card* hold = c[i1];
				c[i1] = c[i1 + 1];
				c[i1 + 1] = hold;
			}
		}
	}

	// Each card'ss rank must be 1 less than the next
	for (int i = 0; i < length - 1; i++)
	{
		if (static_cast<int>(c[i]->rank) != static_cast<int>(c[i + 1]->rank) - 1)
		{
			return false;
		}
	}
	return true;
}

bool check_for_human_play()
{
	for (int i = 0; i < 4; i++)
	{
		if (playerHand[i] != NULL)
		{
			if (playerHand[i]->value + pointCount <= 31)
			{
				return true;
			}
		}
	}
	return false;
}

bool check_for_computer_play()
{
	for (int i = 0; i < 4; i++)
	{
		if (computerHand[i] != NULL)
		{
			if (computerHand[i]->value + pointCount <= 31)
			{
				return true;
			}
		}
	}
	return false;
}

void add_to_pcards(struct Card* c)
{
	imgPlayedCard[playedCardsCount].SetImage(c->icon);
	imgPlayedCard[playedCardsCount].Show();
	playedCardsCount += 1;
	pointCount += c->value;
	update_points(pointCount);
	for (int i = 0; i < 8; i++)
		if (playedCards[i] == NULL)
		{
			playedCards[i] = c;
			playedCards[i + 1] = NULL;
			return;
		}
}

void display_message(const String& message)
{
	//    Notify(message);
	auto s = MsgPanelText->GetText();
	MsgPanelText->SetText(s + "\n" + message);
}
