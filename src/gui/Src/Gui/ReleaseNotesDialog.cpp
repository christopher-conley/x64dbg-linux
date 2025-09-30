#include "ReleaseNotesDialog.h"
#include "ui_ReleaseNotesDialog.h"

#include <QRegularExpression>
#include <QDesktopServices>
#include <QResizeEvent>
#include <QScrollBar>

#include <md4c-html.h>

struct BadEmoji
{
    QString original;
    QString replacement;

    BadEmoji(const char* emoji) // NOLINT(google-explicit-constructor)
        : original(QString::fromUtf8(emoji))
    {
        replacement = QString("<span class=\"emoji\">%1</span>").arg(original);
    }
};

static QString fixupHtmlEmojiBug(QString html)
{
    // Certain emojis do not display correctly on Windows with the Segoe UI font.
    // As a workaround we enclose them in a <span class="emoji">
#ifdef Q_OS_WINDOWS
    static BadEmoji badEmojis[] =
    {
        "\xe2\x98\xba", // ☺
        "\xe2\x98\xb9", // ☹
        "\xe2\x98\xa0", // ☠
        "\xe2\x9d\xa3", // ❣
        "\xe2\x9d\xa4", // ❤
        "\xf0\x9f\x97\xa8", // 🗨
        "\xe2\x9c\x8c", // ✌
        "\xe2\x98\x9d", // ☝
        "\xe2\x9c\x8d", // ✍
        "\xe2\x99\xa8", // ♨
        "\xe2\x9c\x88", // ✈
        "\xe2\x98\x80", // ☀
        "\xe2\x98\x81", // ☁
        "\xe2\x9d\x84", // ❄
        "\xe2\x98\x84", // ☄
        "\xe2\x99\xa0", // ♠
        "\xe2\x99\xa5", // ♥
        "\xe2\x99\xa6", // ♦
        "\xe2\x99\xa3", // ♣
        "\xe2\x99\x9f", // ♟
        "\xe2\x9c\x8f", // ✏
        "\xe2\x9c\x92", // ✒
        "\xe2\x9c\x82", // ✂
        "\xe2\x98\xa2", // ☢
        "\xe2\x98\xa3", // ☣
        "\xe2\x86\x97", // ↗
        "\xe2\x9e\xa1", // ➡
        "\xe2\x86\x98", // ↘
        "\xe2\x86\x99", // ↙
        "\xe2\x86\x96", // ↖
        "\xe2\x86\x95", // ↕
        "\xe2\x86\x94", // ↔
        "\xe2\x86\xa9", // ↩
        "\xe2\x86\xaa", // ↪
        "\xe2\xa4\xb4", // ⤴
        "\xe2\xa4\xb5", // ⤵
        "\xe2\x9c\xa1", // ✡
        "\xe2\x98\xb8", // ☸
        "\xe2\x98\xaf", // ☯
        "\xe2\x9c\x9d", // ✝
        "\xe2\x98\xa6", // ☦
        "\xe2\x98\xaa", // ☪
        "\xe2\x98\xae", // ☮
        "\xe2\x99\x88", // ♈
        "\xe2\x99\x89", // ♉
        "\xe2\x99\x8a", // ♊
        "\xe2\x99\x8b", // ♋
        "\xe2\x99\x8c", // ♌
        "\xe2\x99\x8d", // ♍
        "\xe2\x99\x8e", // ♎
        "\xe2\x99\x8f", // ♏
        "\xe2\x99\x90", // ♐
        "\xe2\x99\x91", // ♑
        "\xe2\x99\x92", // ♒
        "\xe2\x99\x93", // ♓
        "\xe2\x96\xb6", // ▶
        "\xe2\x97\x80", // ◀
        "\xe2\x99\x80", // ♀
        "\xe2\x99\x82", // ♂
        "\xe2\x9a\xa7", // ⚧
        "\xe2\x9c\x96", // ✖
        "\xe2\x80\xbc", // ‼
        "\xe2\x81\x89", // ⁉
        "\xe3\x80\xb0", // 〰
        "\xe2\x98\x91", // ☑
        "\xe2\x9c\x94", // ✔
        "\xe3\x80\xbd", // 〽
        "\xe2\x9c\xb3", // ✳
        "\xe2\x9c\xb4", // ✴
        "\xe2\x9d\x87", // ❇
        "\xe2\x84\xa2", // ™
        "\xe2\x93\x82", // Ⓜ
        "\xf0\x9f\x88\x81", // 🈁
        "\xf0\x9f\x88\x82", // 🈂
        "\xf0\x9f\x88\xb7", // 🈷
        "\xf0\x9f\x88\xb6", // 🈶
        "\xf0\x9f\x88\xaf", // 🈯
        "\xf0\x9f\x89\x90", // 🉐
        "\xf0\x9f\x88\xb9", // 🈹
        "\xf0\x9f\x88\x9a", // 🈚
        "\xf0\x9f\x88\xb2", // 🈲
        "\xf0\x9f\x89\x91", // 🉑
        "\xf0\x9f\x88\xb8", // 🈸
        "\xf0\x9f\x88\xb4", // 🈴
        "\xf0\x9f\x88\xb3", // 🈳
        "\xe3\x8a\x97", // ㊗
        "\xe3\x8a\x99", // ㊙
        "\xf0\x9f\x88\xba", // 🈺
        "\xf0\x9f\x88\xb5", // 🈵
        "\xe2\x97\xbc", // ◼
        "\xe2\x98\x82", // ☂
        "\xe2\x9c\x89", // ✉
        "\xe2\x96\xab", // ▫
        "\xe2\x96\xaa", // ▪
        "\xe2\x97\xbd", // ◽
        "\xe2\x97\xbe", // ◾
    };
    for(const auto & badEmoji : badEmojis)
    {
        html = html.replace(badEmoji.original, badEmoji.replacement);
    }
#endif // Q_OS_WINDOWS

    // For some reason surrogates do not always display correctly in HTML elements.
    // As a workaround we add a zero-width space in front of the high surrogate.
    QString result;
    int size = html.size();
    result.reserve(size);
    for(int i = 0; i < size; i++)
    {
        auto ch = html.at(i);
        if(ch.isHighSurrogate())
        {
            result += QChar(0x200B);
            for(; i < size; i++)
            {
                ch = html.at(i);
                result += ch;
                if(ch.unicode() < 0x0080)
                {
                    break;
                }
            }
        }
        else
        {
            result += ch;
        }
    }
    return result;
}

static QString markdownToHtml(const QString & markdown)
{
    auto appendString = [](const MD_CHAR * text, MD_SIZE size, void* userdata)
    {
        ((std::string*)userdata)->append(text, size);
    };
    std::string html = "<body>";
    unsigned int parserFlags =
        MD_FLAG_COLLAPSEWHITESPACE |
        MD_FLAG_TABLES |
        MD_FLAG_STRIKETHROUGH |
        MD_FLAG_TASKLISTS |
        MD_FLAG_PERMISSIVEAUTOLINKS |
        MD_FLAG_LATEXMATHSPANS;
    unsigned int rendererFlags = 0;
    auto markdownUtf8 = markdown.toUtf8();
    if(md_html(markdownUtf8.constData(), markdownUtf8.size(), appendString, &html, parserFlags, rendererFlags) != 0)
    {
        return {};
    }
    html += "</body>";
    return QString::fromStdString(html);
}

static void markdownGithubLinks(QString & markdown, const QString & issueUrl)
{
    static QRegularExpression usernameRegex(R"((?<=^|\s)@([a-zA-Z0-9-]{1,39})(?=\s|$))");
    markdown.replace(usernameRegex, R"([@\1](https://github.com/\1))");
    if(!issueUrl.isEmpty())
    {
        static QRegularExpression issueRegex(R"((?<=^|\s)#(\d+)(?=\s|$))");
        markdown.replace(issueRegex, QString(R"([#\1](%1\1))").arg(issueUrl));
    }
}

ReleaseNotesDialog::ReleaseNotesDialog(ImageTextBrowser::DownloadFn downloadFn, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::ReleaseNotesDialog)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    ui->setupUi(this);
    ui->textBrowser->setDownloadFn(std::move(downloadFn));

#ifdef Q_OS_WINDOWS
    QFont font("Segoe UI");
#else
    QFont font = QApplication::font();
#endif // Q_OS_WINDOWS
    font.setHintingPreference(QFont::PreferNoHinting);
    font.setPixelSize(16);
    font.setStyleHint(QFont::SansSerif);
    ui->textBrowser->document()->setDefaultFont(font);

    QPalette palette = ui->textBrowser->palette();
    if(palette.color(QPalette::Text) == Qt::black)
    {
        palette.setColor(QPalette::Text, QColor("#1f2328"));
        ui->textBrowser->setPalette(palette);
    }

    ui->textBrowser->document()->setDocumentMargin(14);

    QString styleSheet = R"(
h1, h2, h3, h4, h5, h6, strong, b {
  font-weight: 500;
})";
#ifdef Q_OS_WINDOWS
    styleSheet += R"(
.emoji {
  font-family: "Segoe UI Emoji";
})";
#endif // Q_OS_WINDOWS

    ui->textBrowser->document()->setDefaultStyleSheet(styleSheet);
    ui->textBrowser->setOpenLinks(false);
    connect(ui->textBrowser, &QTextBrowser::anchorClicked, this, [this](const QUrl & url)
    {
        ui->textBrowser->clearFocus();
        QDesktopServices::openUrl(url);
    });
}

ReleaseNotesDialog::~ReleaseNotesDialog()
{
    delete ui;
}

bool ReleaseNotesDialog::setMarkdown(QString markdown, const QString & issueUrl)
{
    markdownGithubLinks(markdown, issueUrl);
    auto html = markdownToHtml(markdown);
    html = fixupHtmlEmojiBug(html);
    ui->textBrowser->setText(html);
    return !html.isEmpty();
}

void ReleaseNotesDialog::setLabel(const QString & text)
{
    ui->label->setText(text);
}

void ReleaseNotesDialog::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    ui->textBrowser->resizeImages();
}
