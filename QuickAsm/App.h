#pragma once


class App : public wxApp {
public:
	bool OnInit() override;
	int OnExit() override;

	bool Restart{ false };
private:
};

wxDECLARE_APP(App);
