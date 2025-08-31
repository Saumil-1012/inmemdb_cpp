
#include <QApplication>
#include <QWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QString>

#include <cctype>
#include <sstream>

#include "lib.hpp"

class SqlWindow : public QWidget {
public:
    SqlWindow() {
        setWindowTitle(QStringLiteral("In‑Memory SQL Database"));
        auto* layout = new QVBoxLayout(this);
        input_ = new QTextEdit(this);
        input_->setPlaceholderText(QStringLiteral("Write SQL here; separate statements with ;"));
        output_ = new QTextEdit(this);
        output_->setReadOnly(true);
        auto* runButton = new QPushButton(QStringLiteral("Run"), this);
        layout->addWidget(input_);
        layout->addWidget(runButton);
        layout->addWidget(output_);
        connect(runButton, &QPushButton::clicked, this, &SqlWindow::runQueries);
    }

private:
    void runQueries() {
        using namespace db;
        std::string all = input_->toPlainText().toStdString();
        std::ostringstream out;
        std::ostringstream err;
        // Process statements one by one.  The same logic as the CLI
        // applies: split at semicolons outside of quotes, then parse
        // and execute.  Output is appended to the output pane.
        for (const auto& stmt_src : split_statements(all)) {
            bool only_ws = true;
            for (unsigned char c : stmt_src) {
                if (!std::isspace(c)) { only_ws = false; break; }
            }
            if (only_ws) continue;
            try {
                auto toks = tokenize(stmt_src);
                TokenStream ts{toks};
                auto stmt = parse_statement(ts);
                if (!stmt) continue;
                if (auto* s = dynamic_cast<Select*>(stmt.get())) {
                    auto res = db_.exec(*s);
                    out << render_ascii(res);
                } else if (auto* s = dynamic_cast<CreateTable*>(stmt.get())) {
                    db_.exec(*s);
                } else if (auto* s = dynamic_cast<Insert*>(stmt.get())) {
                    db_.exec(*s);
                } else if (auto* s = dynamic_cast<Delete*>(stmt.get())) {
                    db_.exec(*s);
                } else if (auto* s = dynamic_cast<Update*>(stmt.get())) {
                    db_.exec(*s);
                } else if (auto* s = dynamic_cast<Dump*>(stmt.get())) {
                    db_.exec(*s);
                } else if (auto* s = dynamic_cast<Load*>(stmt.get())) {
                    db_.exec(*s);
                } else if (auto* s = dynamic_cast<DropTable*>(stmt.get())) {
                    db_.exec(*s);
                }
            } catch (const std::exception& ex) {
                err << "Error: " << ex.what() << "\n";
            }
        }
        output_->setPlainText(QString::fromStdString(out.str() + err.str()));
    }

private:
    QTextEdit* input_{};
    QTextEdit* output_{};
    db::Database db_{};
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    SqlWindow w;
    w.resize(800, 600);
    w.show();
    return app.exec();
}
