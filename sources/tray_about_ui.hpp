/* I was getting SIIICK of debugging UI issues with ui files, so fuck it. */
#ifndef ABOUTVWVSYB_H
#define ABOUTVWVSYB_H

#include "raylib.h"
#include <QScrollArea>
#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <qwidget.h>

QT_BEGIN_NAMESPACE

class Ui_LinkNameAndDescWidget
{
  public:
	QHBoxLayout *horizontalLayout;
	QLabel *namendesc;
	QPushButton *urlButton;

	void setupUi(QWidget *widget, QString name, QString version, QString desc, QString link, bool credit = false)
	{
		if (widget->objectName().isEmpty())
			widget->setObjectName("widget");
		widget->setWindowModality(Qt::WindowModality::NonModal);
		widget->setEnabled(true);
		// widget->resize(640, 66);
		horizontalLayout = new QHBoxLayout(widget);
		horizontalLayout->setObjectName("horizontalLayout");
		namendesc = new QLabel(widget);
		namendesc->setObjectName("namendesc");

		horizontalLayout->addWidget(namendesc);

		urlButton = new QPushButton(widget);
		urlButton->setObjectName("urlButton");
		urlButton->setMaximumSize(QSize(32, 32));
		QIcon icon(QIcon::fromTheme(QIcon::ThemeIcon::HelpFaq));
		urlButton->setIcon(icon);
		urlButton->setFlat(true);

		horizontalLayout->addWidget(urlButton);

		retranslateUi(widget, name, version, desc, link, credit);
		if (link.isEmpty())
			urlButton->hide();

		QMetaObject::connectSlotsByName(widget);
	} // setupUi

	void retranslateUi(QWidget *widget, QString name, QString version, QString desc, QString link, bool credit)
	{
		// widget->setWindowTitle(QCoreApplication::translate("widget", "Form", nullptr));
		const char *fmt = "<html><head/><body><p><span style=\" font-weight:700;\">%s</span>%s<br>%s</p></body></html>";
		namendesc->setText(QCoreApplication::translate("widget", TextFormat(fmt, name.toStdString().c_str(), !version.isEmpty() ? (" (" + version.toStdString() + ")").c_str() : "", desc.toStdString().c_str()), nullptr));
		urlButton->setText(QString());
		if (!credit)
			urlButton->setToolTip(QString(TextFormat("Visit component's homepage\n%s", link.toStdString().c_str())));
		else
			urlButton->setToolTip(QString(TextFormat("Visit author's homepage\n%s", link.toStdString().c_str())));
		widget->connect(urlButton, &QPushButton::clicked, [&]() { OpenURL(link.toStdString().c_str()); });
	} // retranslateUi
};

class Ui_AboutDeltatunix
{
  public:
	QVBoxLayout *verticalLayout_1;
	QVBoxLayout *verticalLayout_2;
	QVBoxLayout *verticalLayout_3;
	QVBoxLayout *verticalLayout_4;
	QFormLayout *formLayout;
	QLabel *label_2;
	QLabel *label_4;
	QTabWidget *atabWidget;
	QWidget *aboutTab;
	QScrollArea *aboutScrollArea;
	QLabel *label;
	QWidget *componentsTab;
	QScrollArea *componentsScrollArea;
	QWidget *creditsTab;
	QScrollArea *creditsScrollArea;
	QDialogButtonBox *buttonBox;
	QHBoxLayout *horizontalLayout;
	QPushButton *aboutQtButton;
	QSpacerItem *buttonSpacer;
	QPushButton *closeButton;

	std::vector<QWidget *> componentWidgets;

	void setupComponentWidget(QWidget *parent, QString name, QString version, QString desc, QString link, bool credit = false)
	{
		Ui_LinkNameAndDescWidget comp;
		QWidget *widget = new QWidget(parent);
		comp.setupUi(widget, name, version, desc, link, credit);
		componentWidgets.push_back(widget);
	}

	void setupUi(QDialog *AboutDeltatunix)
	{
		if (AboutDeltatunix->objectName().isEmpty())
			AboutDeltatunix->setObjectName("AboutDeltatunix");
		AboutDeltatunix->resize(562, 326);
		QIcon icon;
		icon.addFile(QString::fromUtf8(":/icon.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
		AboutDeltatunix->setWindowIcon(icon);
		verticalLayout_2 = new QVBoxLayout(AboutDeltatunix);
		verticalLayout_2->setObjectName("verticalLayout_2");
		formLayout = new QFormLayout();
		formLayout->setObjectName("formLayout");
		label_2 = new QLabel(AboutDeltatunix);
		label_2->setObjectName("label_2");

		formLayout->setWidget(1, QFormLayout::ItemRole::LabelRole, label_2);

		label_4 = new QLabel(AboutDeltatunix);
		label_4->setObjectName("label_4");
		label_4->setAlignment(Qt::AlignmentFlag::AlignLeading | Qt::AlignmentFlag::AlignLeft | Qt::AlignmentFlag::AlignTop);

		formLayout->setWidget(1, QFormLayout::ItemRole::FieldRole, label_4);

		verticalLayout_2->addLayout(formLayout);

		atabWidget = new QTabWidget(AboutDeltatunix);
		atabWidget->setObjectName("atabWidget");
		QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
		sizePolicy.setHorizontalStretch(1);
		sizePolicy.setVerticalStretch(1);
		sizePolicy.setHeightForWidth(atabWidget->sizePolicy().hasHeightForWidth());
		atabWidget->setSizePolicy(sizePolicy);
		atabWidget->setMinimumSize(QSize(206, 35));
		atabWidget->setMaximumSize(QSize(16777215, 16777215));
		atabWidget->setTabPosition(QTabWidget::TabPosition::West);
		atabWidget->setTabShape(QTabWidget::TabShape::Rounded);

		aboutTab = new QWidget();
		aboutTab->setObjectName("aboutTab");
		label = new QLabel(aboutTab);
		label->setObjectName("label");
		label->setAlignment(Qt::AlignmentFlag::AlignLeading | Qt::AlignmentFlag::AlignLeft | Qt::AlignmentFlag::AlignTop);
		label->setWordWrap(true);
		label->setOpenExternalLinks(true);

		aboutScrollArea = new QScrollArea();
		aboutScrollArea->setWidgetResizable(true);

		verticalLayout_1 = new QVBoxLayout(aboutTab);
		verticalLayout_1->addWidget(label);

		aboutScrollArea->setWidget(aboutTab);

		atabWidget->addTab(aboutScrollArea, QString("About"));

		componentsScrollArea = new QScrollArea();
		componentsScrollArea->setWidgetResizable(true);

		componentsTab = new QWidget(componentsScrollArea);
		componentsTab->setObjectName("componentsTab");
		verticalLayout_3 = new QVBoxLayout(componentsTab);

		setupComponentWidget(componentsTab, "raylib", "6.0", "A programming library to enjoy videogames programming", "https://www.raylib.com/");
		setupComponentWidget(componentsTab, "sdbus-c++", "2.3.1", "High-level C++ D-Bus library for Linux", "https://github.com/Kistler-Group/sdbus-cpp/");
		setupComponentWidget(componentsTab, "ini.h", "1.2", "Simple ini-file reader for C/C++", "https://github.com/mattiasgustavsson/libs/");
		setupComponentWidget(componentsTab, "Qt", qVersion(), "Cross-platform application development framework.", "https://qt.io/");

		for (const auto &widget : componentWidgets)
		{
			verticalLayout_3->addWidget(widget);
		}

		componentsScrollArea->setWidget(componentsTab);
		atabWidget->addTab(componentsScrollArea, QString("Components"));

		creditsScrollArea = new QScrollArea();
		creditsScrollArea->setWidgetResizable(true);

		componentWidgets.clear();

		creditsTab = new QWidget(creditsScrollArea);
		creditsTab->setObjectName("creditsTab");
		verticalLayout_4 = new QVBoxLayout(creditsTab);

		setupComponentWidget(creditsTab, "Hayasaka \"Ellen\" Yonii", "", "Main programmer", "https://haya3218.nekoweb.org/", true);
		setupComponentWidget(creditsTab, "ToastworthLP", "", "Creator of DeltaTune", "https://x.com/Toastworth_", true);
		setupComponentWidget(creditsTab, "Citra", "", "Base font", "", true);
		setupComponentWidget(creditsTab, "Yasuyuki Furukawa", "", "Japanese font (Shinonome)", "http://jikasei.me/font/jf-dotfont", true);
		setupComponentWidget(creditsTab, "Toby Fox", "", "... For DELTARUNE.", "https://deltarune.com/", true);

		for (const auto &widget : componentWidgets)
		{
			verticalLayout_4->addWidget(widget);
		}

		creditsScrollArea->setWidget(creditsTab);

		atabWidget->addTab(creditsScrollArea, QString("Credits"));

		verticalLayout_2->addWidget(atabWidget);

		horizontalLayout = new QHBoxLayout();
		horizontalLayout->setObjectName("horizontalLayout");
		aboutQtButton = new QPushButton(AboutDeltatunix);
		aboutQtButton->setObjectName("aboutQtButton");

		QSizePolicy sizePolicy1(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);
		sizePolicy1.setHorizontalStretch(0);
		sizePolicy1.setVerticalStretch(0);
		sizePolicy1.setHeightForWidth(aboutQtButton->sizePolicy().hasHeightForWidth());
		aboutQtButton->setSizePolicy(sizePolicy1);
		aboutQtButton->setLayoutDirection(Qt::LayoutDirection::LeftToRight);

		horizontalLayout->addWidget(aboutQtButton);

		buttonSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

		horizontalLayout->addItem(buttonSpacer);

		closeButton = new QPushButton(AboutDeltatunix);
		closeButton->setObjectName("closeButton");
		sizePolicy1.setHeightForWidth(closeButton->sizePolicy().hasHeightForWidth());
		closeButton->setSizePolicy(sizePolicy1);
		QIcon icon1(QIcon::fromTheme(QIcon::ThemeIcon::WindowClose));
		closeButton->setIcon(icon1);

		horizontalLayout->addWidget(closeButton);

		verticalLayout_2->addLayout(horizontalLayout);

		retranslateUi(AboutDeltatunix);
		// QObject::connect(buttonBox, &QDialogButtonBox::accepted, AboutDeltatunix, qOverload<>(&QDialog::accept));
		// QObject::connect(buttonBox, &QDialogButtonBox::rejected, AboutDeltatunix, qOverload<>(&QDialog::reject));

		atabWidget->setCurrentIndex(0);

		QMetaObject::connectSlotsByName(AboutDeltatunix);
	} // setupUi

	void retranslateUi(QDialog *AboutDeltatunix)
	{
		AboutDeltatunix->setWindowTitle(QCoreApplication::translate("AboutDeltatunix", "About DeltaTunix", nullptr));
		label_2->setText(QCoreApplication::translate("AboutDeltatunix", "<html><head/><body><p><img src=\":/icon_about.png\"/></p></body></html>", nullptr));
		label_4->setText(QCoreApplication::translate("AboutDeltatunix", "<html><head/><body><p><span style=\" font-size:16pt;\">DeltaTunix</span><br>Version 0.0.1</p></body></html>", nullptr));
		label->setText(QCoreApplication::translate("AboutDeltatunix", "<html><head/><body><p>DeltaTunix is a clone of <a href=\"https://github.com/ToadsworthLP/deltatune\"><span style=\" text-decoration: underline; color:#0000ff;\">DeltaTune</span></a> by Toastworth that brings a Deltarune style music indicator to the desktop.</p><p>(c) 2026 Haya</p><p>DELTARUNE was created by Toby Fox.<br>Not affiliated with or endorsed by Toby Fox.</p><p><a href=\"https://github.com/hayaunderscore/deltatunix\"><span style=\" text-decoration: underline; color:#0000ff;\">https://github.com/hayaunderscore/deltatunix</span></a><br><a href=\"https://zlib.net/zlib_license.html\"><span style=\" text-decoration: underline; color:#0000ff;\">Licensed under the zlib license.</span></a></p></body></html>", nullptr));
		atabWidget->setTabText(atabWidget->indexOf(componentsTab), QCoreApplication::translate("AboutDeltatunix", "Components", nullptr));
		atabWidget->setTabText(atabWidget->indexOf(creditsTab), QCoreApplication::translate("AboutDeltatunix", "Credits", nullptr));
		aboutQtButton->setText(QCoreApplication::translate("AboutDeltatunix", "About Qt", nullptr));
		closeButton->setText(QCoreApplication::translate("AboutDeltatunix", "Close", nullptr));
		QObject::connect(aboutQtButton, &QPushButton::clicked, [&]() { QApplication::aboutQt(); });
	} // retranslateUi
};

namespace Ui
{
class AboutDeltatunix : public Ui_AboutDeltatunix
{
};
class LinkNameAndDescWidget : public Ui_LinkNameAndDescWidget
{
};
} // namespace Ui

QT_END_NAMESPACE

#endif // ABOUTVWVSYB_H
