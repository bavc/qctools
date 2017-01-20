#include <CMarkdown.h>
#include <QFile>
#include <QTextStream>
#include <iostream>
#include <cassert>

CMarkdown::
CMarkdown()
{
}

void
CMarkdown::
setDebug(bool d)
{
  debug_ = d;
}

QString
CMarkdown::
processFile(const QString &filename)
{
  QFile file(filename);

  if (! file.open(QFile::ReadOnly | QFile::Text))
    return "";

  QTextStream stream(&file);

  return processText(stream.readAll());
}

QString
CMarkdown::
processText(const QString &str)
{
  delete rootBlock_;

  rootBlock_ = new CMarkdownBlock(this);

  //---

  str_ = str;
  pos_ = 0;
  len_ = str_.length();

  QString line;

  while (readLine(line))
    rootBlock_->addLine(line);

  return rootBlock_->process();
}

void
CMarkdown::
addLink(const LinkRef &link)
{
  links_[link.ref] = link;
}

bool
CMarkdown::
getLink(const QString &ref, LinkRef &link) const
{
  auto p = links_.find(ref);

  if (p == links_.end())
    return false;

  link = (*p).second;

  return true;
}

bool
CMarkdown::
readLine(QString &line)
{
  if (pos_ >= len_)
    return false;

  line = "";

  while (pos_ < len_ && str_[pos_] != '\n')
    line += str_[pos_++];

  if (pos_ < len_ && str_[pos_] == '\n')
    ++pos_;

  return true;
}

//------

CMarkdownBlock::
CMarkdownBlock(CMarkdown *markdown) :
 markdown_(markdown), parent_(0), type_(BlockType::DOCUMENT)
{
}

CMarkdownBlock::
CMarkdownBlock(CMarkdownBlock *parent, BlockType type) :
 markdown_(0), parent_(parent), type_(type)
{
}

CMarkdownBlock::
~CMarkdownBlock()
{
  for (auto &b : blocks_)
    delete b;
}

void
CMarkdownBlock::
addBlock(CMarkdownBlock *block)
{
  blocks_.push_back(block);
}

void
CMarkdownBlock::
addLine(const Line &line)
{
  lines_.push_back(line);
}

void
CMarkdownBlock::
appendLine(const QString &line)
{
  assert(! lines_.empty());

  lines_.back().line += line;
}

QString
CMarkdownBlock::
process()
{
  if (! isRecurseType(type_) || processed_)
    return html_;

  currentLine_  = 0;

  rootBlock_    = this;
  currentBlock_ = rootBlock_;

  html_ = processLines();

  processed_ = true;

  return html_;
}

QString
CMarkdownBlock::
processLines()
{
  QString html = "";

  int       indent;
  QChar     c;
  QString   text;
  BlockType type;
  LinkRef   linkRef;
  ListData  list;

  while (currentLine_ < int(lines_.size())) {
    // read line (tabs converted to 4 spaces)
    LineData line1;

    if (! getLine(line1))
      break;

    if (markdown()->isDebug())
      std::cerr << "Line: '" << line1.line.toStdString() << "'" << std::endl;

    //---

    CodeFence fence;

    if      (isBlankLine(line1.line)) {
      endBlock();
    }
    else if (isStartCodeFence(line1.line, fence)) {
      flushBlocks();

      CMarkdownBlock *block = startBlock(BlockType::PRE);

      startBlock(BlockType::CODE);

      LineData line2;

      while (getLine(line2)) {
        if (isEndCodeFence(line2.line, fence))
          break;

        addBlockLine(line2.line);
      }

      endBlock();
      endBlock();

      html += block->toHtml();
    }
    else if (isRule(line1.line)) {
      endBlock();

      CMarkdownBlock *block = startBlock(BlockType::HR);

      endBlock();

      html += block->toHtml();
    }
    else if (isHtmlLine(line1.line)) {
      flushBlocks();

      html += line1.line + "\n";

      LineData line2;

      while (getLine(line2)) {
        if (isBlankLine(line2.line))
          break;

        html += line2.line + "\n";
      }

      html += "\n";
    }
    else if (isLinkReference(line1.line, linkRef)) {
      endBlock();

      markdown()->addLink(linkRef);
    }
    else if (isUnorderedListLine(line1.line, list)) {
      endBlock();

      CMarkdownBlock *block = startBlock(BlockType::UL);

      startBlock(BlockType::LI);

      addBlockLine(list.text);

      int bl = 0;

      LineData line2;

      while (getLine(line2)) {
        ListData list1;

        if      (line2.indent >= list.indent) {
          addBlockLine(line2.line.mid(list.indent));

          bl = 0;
        }
        else if (isUnorderedListLine(line2.line, list1)) {
          if (bl > 0) {
            endBlock();

            startBlock(BlockType::LI);
          }

          if (list1.indent >= list.indent && list1.c == list.c) {
            endBlock();

            startBlock(BlockType::LI);

            addBlockLine(list1.text);
          }
          else {
            ungetLine();
            break;
          }

          bl = 0;
        }
        else if (isBlankLine(line2.line)) {
          ++bl;

          if (bl > 1)
            break;
        }
        else {
          ungetLine();
          break;
        }
      }

      endBlock();
      endBlock();

      html += block->toHtml();
    }
    else if (isOrderedListLine(line1.line, list)) {
      endBlock();

      CMarkdownBlock *block = startBlock(BlockType::OL);

      startBlock(BlockType::LI);

      addBlockLine(list.text);

      int bl = 0;

      LineData line2;

      while (getLine(line2)) {
        ListData list1;

        if      (line2.indent >= list.indent) {
          addBlockLine(line2.line.mid(list.indent));

          bl = 0;
        }
        else if (isOrderedListLine(line2.line, list1)) {
          if (bl > 0) {
            // add empty list item
            endBlock();

            startBlock(BlockType::LI);
          }

          if (list1.indent >= list.indent && list1.c == list.c) {
            endBlock();

            startBlock(BlockType::LI);

            addBlockLine(list1.text);
          }
          else {
            ungetLine();
            break;
          }

          bl = 0;
        }
        else if (isBlankLine(line2.line)) {
          ++bl;

          if (bl > 1)
            break;
        }
        else {
          ungetLine();
          break;
        }
      }

      endBlock();
      endBlock();

      html += block->toHtml();
    }
    else if (isATXHeader(line1.line, type, text)) {
      endBlock();

      CMarkdownBlock *block = startBlock(type);

      addBlockLine(text);

      endBlock();

      html += block->toHtml();
    }
    else if (isIndentLine(line1.line, indent)) {
      flushBlocks();

      CMarkdownBlock *block = startBlock(BlockType::PRE);

      startBlock(BlockType::CODE);

      addBlockLine(line1.line.mid(indent));

      LineData line2;

      while (getLine(line2)) {
        if      (isIndentLine(line2.line, indent))
          addBlockLine(line2.line.mid(indent));
        else if (isBlankLine(line2.line))
          addBlockLine(line2.line);
        else {
          ungetLine();
          break;
        }
      }

      endBlock();
      endBlock();

      html += block->toHtml();
    }
    else if (isBlockQuote(line1.line, text)) {
      CMarkdownBlock *block = startBlock(BlockType::BLOCKQUOTE);

      addBlockLine(text);

      LineData line2;

      QString quote1;

      while (getLine(line2)) {
        if      (isContinuationLine(line2.line)) {
          appendBlockLine(line2.line);
        }
        else if (isBlockQuote(line2.line, quote1)) {
          addBlockLine(quote1);
        }
        else {
          ungetLine();
          break;
        }
      }

      endBlock();

      html += block->toHtml();
    }
    else if (isTableLine(line1.line)) {
      CMarkdownBlock *block = startBlock(BlockType::TABLE);

      parseTableLine(line1.line);

      LineData line2;

      while (getLine(line2)) {
        if (isTableLine(line2.line))
          parseTableLine(line2.line);
        else {
          ungetLine();
          break;
        }
      }

      endBlock();

      html += block->toHtml();
    }
    else {
      endBlock();

      CMarkdownBlock *block = startBlock(BlockType::P);

      addBlockLine(line1.line, line1.brk);

      int nl = 0;

      LineData line2;

      while (getLine(line2)) {
        if (isBlankLine(line2.line))
          break;

        BlockType type;

        if      (nl == 0 && isSetTextLine(line2.line, type)) {
          endBlock(); // remove block

          int i = 0;

          skipSpace(line1.line, i);

          if (i > 0)
            line1.line = line1.line.mid(i);

          CMarkdownBlock *block = startBlock(type);

          addBlockLine(line1.line);

          endBlock();

          html += block->toHtml();

          nl = -1;

          break;
        }
        else if (isFormatLine(line2.line)) {
          ungetLine();
          break;
        }

        addBlockLine(line2.line.mid(line2.indent), line2.brk);

        ++nl;
      }

      if (nl >= 0) {
        endBlock();

        html += block->toHtml();
      }
    }
  }

  endBlock();

  return html;
}

bool
CMarkdownBlock::
isContinuationLine(const QString &str) const
{
  int len = str.length();

  if (len == 0 || isASCIIPunct(str[0]))
    return false;

  return true;
}

bool
CMarkdownBlock::
isBlankLine(const QString &str) const
{
  // An empty line, or a line containing only spaces or tabs, is a blank line.
  int i = 0;

  if (skipSpace(str, i) != str.length())
    return false;

  return true;
}

bool
CMarkdownBlock::
isRule(const QString &str) const
{
  int len = str.length();

  int i = 0;

  // max of three spaces indent
  if (skipSpace(str, i) >= 4)
    return false;

  // check for rule characters
  QChar c = str[i];

  if (c != '-' && c != '*' && c != '_')
    return false;

  ++i;

  // check for three or more matching characters (with spaces between)
  int nc = 1;

  while (i < len) {
    skipSpace(str, i);

    if (str[i] != c)
      return false;

    ++i; ++nc;
  }

  return (nc >= 3);
}

bool
CMarkdownBlock::
isATXHeader(const QString &str, CMarkdownBlock::BlockType &type, QString &text) const
{
  int len = str.length();

  int i = 0;

  // up to 3 spaces
  if (skipSpace(str, i) > 3)
    return false;

  // followed by hash (max of 6 hashes)
  int nh = skipChar(str, i, '#');

  if (nh < 1 || nh > 6)
    return false;

  // followed by a space or end of line
  if (i < len && ! str[i].isSpace())
    return false;

  // get header type
  if      (nh == 1) type = CMarkdownBlock::BlockType::H1;
  else if (nh == 2) type = CMarkdownBlock::BlockType::H2;
  else if (nh == 3) type = CMarkdownBlock::BlockType::H3;
  else if (nh == 4) type = CMarkdownBlock::BlockType::H4;
  else if (nh == 5) type = CMarkdownBlock::BlockType::H5;
  else if (nh == 6) type = CMarkdownBlock::BlockType::H6;

  // skip spaces after '#'
  skipSpace(str, i);

  // get remaining text
  text = str.mid(i);

  // remove trailing '#'
  if (text != "") {
    if (text[0] == '#')
      text = " " + text;

    int len1 = text.length();

    int i1 = len1 - 1;

    backSkipSpace(text, i1);

    backSkipChar(text, i1, '#');

    if (i1 >= 0 && text[i1].isSpace()) {
      backSkipSpace(text, i1);

      text = text.mid(0, i1 + 1);
    }
  }

  return true;
}

bool
CMarkdownBlock::
isSetTextLine(const QString &str, CMarkdownBlock::BlockType &type) const
{
  int i   = 0;
  int len = str.length();

  // up to 3 spaces
  if (skipSpace(str, i) > 3)
    return false;

  // followed by '=' or '-'
  if (i >= len || (str[i] != '=' && str[i] != '-'))
    return false;

  QChar c = str[i];

  (void) skipChar(str, i, c);

  (void) skipSpace(str, i);

  // must be end of line
  if (i < len)
    return false;

  type = (c == '=' ? CMarkdownBlock::BlockType::H1 : CMarkdownBlock::BlockType::H2);

  return true;
}

bool
CMarkdownBlock::
isIndentLine(const QString &str, int &n) const
{
  int len = str.length();

  int i = 0;

  n = skipSpace(str, i);

  if (n >= 4 && i != len)
    return true;

  return false;
}

bool
CMarkdownBlock::
isFormatLine(const QString &str) const
{
  int len = str.length();

  int i = 0;

  // skip space and detect indent line
  if (skipSpace(str, i) >= 4 && i != len)
    return true;

  // starts with format char
  if (! isFormatChar(str, i))
    return false;

  // has any format chars
  for ( ; i < len; ++i)
    if (isFormatChar(str, i))
      return true;

  return false;
}

bool
CMarkdownBlock::
isFormatChar(const QString &str, int i) const
{
  if (str[i] == '>') return true; // block quote
  if (str[i] == '+') return true; // unordered list
  if (str[i] == '-') return true; // unordered list
  if (str[i] == '*') return true; // unordered list
  if (str[i] == '#') return true; // header
  if (str[i] == '|') return true; // table

  if (str[i] == '`' || str[i] == '~') { // code fence
    int j = i;

    QChar c = str[i];

    if (skipChar(str, j, c) >= 3)
      return true;
  }

  int len = str.length();

  if (i < len && str[i].isDigit()) {
    int j = i + 1;

    while (j < len && str[j].isDigit())
      ++j;

    if (j < len && (str[j] == '.' || str[j] == ')')) // ordered list
      return true;
  }

  return false;
}

bool
CMarkdownBlock::
isStartCodeFence(const QString &str, CodeFence &fence) const
{
  int len = str.length();

  int i = 0;

  if (skipSpace(str, i) > 3)
    return false;

  if (i >= len)
    return false;

  fence.c = str[i++];

  if (fence.c != '`' && fence.c != '~')
    return false;

  fence.n = skipChar(str, i, fence.c) + 1;

  if (fence.n < 3)
    return false;

  skipSpace(str, i);

  fence.info = "";

  while (i < len) {
    if (str[i] == '`')
      return false;

    fence.info += str[i++];
  }

  fence.info = fence.info.simplified();

  return true;
}

bool
CMarkdownBlock::
isEndCodeFence(const QString &str, const CodeFence &fence) const
{
  int len = str.length();

  int i = 0;

  if (skipSpace(str, i) > 3)
    return false;

  if (i >= len)
    return false;

  if (str[i] != fence.c)
    return false;

  ++i;

  if (skipChar(str, i, fence.c) < fence.n - 1)
    return false;

  return true;
}

bool
CMarkdownBlock::
isHtmlLine(const QString &str) const
{
  int len = str.length();

  int i = 0;

  if (skipSpace(str, i) > 3)
    return false;

  if (i >= len)
    return false;

  if (str[i] != '<')
    return false;

  ++i;

  QString name;

  while (i < len && str[i].isLetter())
    name += str[i++];

  if (name == "article"    || name == "header"     || name == "aside"    || name == "hgroup" ||
      name == "blockquote" || name == "hr"         || name == "iframe"   || name == "body" ||
      name == "map"        || name == "button"     || name == "object" ||
      name == "canvas"     || name == "caption"    || name == "output" ||
      name == "col"        || name == "p"          || name == "colgroup" || name == "pre" ||
      name == "dd"         || name == "progress"   || name == "div"      || name == "section" ||
      name == "dl"         || name == "table"      || name == "td"       || name == "dt" ||
      name == "tbody"      || name == "embed"      || name == "textarea" || name == "fieldset" ||
      name == "tfoot"      || name == "figcaption" || name == "th"       || name == "figure" ||
      name == "thead"      || name == "footer"     || name == "tr"       || name == "form" ||
      name == "h1"         || name == "h2"         || name == "h3" ||
      name == "h4"         || name == "h5"         || name == "h6" ||
      name == "ol"         || name == "ul"         || name == "li" ||
      name == "video"      || name == "script"     || name == "style")
    return true;

  return false;
}

bool
CMarkdownBlock::
isLinkReference(const QString &str, LinkRef &link) const
{
  int len = str.length();

  int i = 0;

  if (skipSpace(str, i) > 3)
    return false;

  if (i >= len || str[i] != '[')
    return false;

  ++i;

  link.ref = "";

  while (i < len && str[i] != ']') {
    link.ref += str[i++];
  }

  if (i >= len)
    return false;

  ++i;

  if (i >= len || str[i] != ':')
    return false;

  ++i;

  skipSpace(str, i);

  // TODO: allow line ending ?

  link.dest = "";

  while (i < len && ! str[i].isSpace()) {
    link.dest += str[i++];
  }

  skipSpace(str, i);

  link.title = "";

  if (i < len && (str[i] == '"' || str[i] == '\'')) {
    QChar c = str[i++];

    while (i < len && str[i] != c)
      link.title += str[i++];

    if (str[i] != c)
      return false;

    ++i;
  }

  skipSpace(str, i);

  if (i < len)
    return false;

  return true;
}

bool
CMarkdownBlock::
isBlockQuote(const QString &str, QString &quote) const
{
  int len = str.length();

  int i = 0;

  if (skipSpace(str, i) >= 4)
    return false;

  if (i >= len || str[i] != '>')
    return false;

  ++i;

  if (i < len && str[i].isSpace())
    quote = str.mid(i + 1);
  else
    quote = str.mid(i);

  return true;
}

bool
CMarkdownBlock::
isUnorderedListLine(const QString &str, ListData &list) const
{
  int len = str.length();

  int i = 0;

  if (skipSpace(str, i) >= 4)
    return false;

  if (i >= len - 1)
    return false;

  if (str[i] != '-' && str[i] != '+' && str[i] != '*')
    return false;

  list.c = str[i];

  ++i;

  if (! str[i].isSpace())
    return false;

  ++i;

  skipSpace(str, i);

  list.indent = i;

  list.text = str.mid(i);

  return true;
}

bool
CMarkdownBlock::
isOrderedListLine(const QString &str, ListData &list) const
{
  int len = str.length();

  int i = 0;

  if (skipSpace(str, i) >= 4)
    return false;

  if (i >= len - 2)
    return false;

  if (i >= len || ! str[i].isDigit())
    return false;

  QString num;

  while (i < len && str[i].isDigit())
    num += str[i++];

  list.n = num.toInt();

  if (i >= len || (str[i] != '.' && str[i] != ')'))
    return false;

  list.c = str[i++];

  if (i >= len || ! str[i].isSpace())
    return false;

  ++i;

  skipSpace(str, i);

  list.indent = i;

  list.text = str.mid(i);

  return true;
}

bool
CMarkdownBlock::
isTableLine(const QString &str) const
{
  int len = str.length();

  int i = 0;

  if (skipSpace(str, i) >= 4)
    return false;

  if (i >= len || str[i] != '|')
    return false;

  return true;
}

void
CMarkdownBlock::
parseTableLine(const QString &str)
{
  int len = str.length();

  int i = 0;

  skipSpace(str, i);
  assert(i < 4);

  assert(i < len && str[i] == '|');

  typedef std::vector<QString> Words;

  Words words;

  ++i;

  QString word;

  while (i < len) {
    if (str[i] == '|') {
      ++i;

      words.push_back(word);

      word = "";
    }
    else
      word += str[i++];
  }

  if (words.empty())
    return;

  startBlock(BlockType::TR);

  for (const auto &word : words) {
    startBlock(BlockType::TD);

    addBlockLine(word);

    endBlock();
  }

  endBlock();
}

QString
CMarkdownBlock::
replaceEmbeddedStyles(const QString &str) const
{
  QString str1;

  int i   = 0;
  int len = str.length();

  while (i < len) {
    // escape
    if      (i < len - 1 && str[i] == '\\' && isASCIIPunct(str[i + 1])) {
      ++i;

      if      (str[i] == '<') {
        str1 += "&lt;"; ++i;
      }
      else if (str[i] == '>') {
        str1 += "&gt;"; ++i;
      }
      else if (str[i] == '"') {
        str1 += "&quot;"; ++i;
      }
      else if (str[i] == '&') {
        str1 += "&amp;"; ++i;
      }
      else
        str1 += str[i++];
    }
    // emphasis
    else if (str[i] == '*' || str[i] == '_') {
      QChar c = str[i];

      QString str2;

      int nc = parseSurroundText(str, i, c, str2);

      if (nc > 0) {
        if (nc == 1)
          str1 += QString("<em>%1</em>").arg(str2);
        else
          str1 += QString("<strong>%1</strong>").arg(str2);
      }
      else {
        while (i < len && str[i] == c) {
          str1 += c; ++i;
        }
      }
    }
    // code
    else if (str[i] == '`') {
      QChar c = str[i];

      QString str2;

      int nc = parseSurroundText(str, i, c, str2);

      if (nc > 0) {
        str1 += QString("<code>%1</code>").arg(str2);
      }
      else {
        while (i < len && str[i] == c) {
          str1 += c; ++i;
        }
      }
    }
    // image link
    else if (i < len - 1 && str[i] == '!' && str[i + 1] == '[') {
      int i1 = i;

      i += 2;

      QString str2;

      while (i < len && str[i] != ']')
        str2 += str[i++];

      if (i < len && str[i] == ']') {
        ++i;

        QString str3, str4;

        if      (i < len && str[i] == '(') {
          ++i;

          while (i < len && str[i] != ')') {
            if (str[i] == '\"')
              break;

            str3 += str[i++];
          }

          if (i < len && str[i] == '\"') {
            ++i;

            while (i < len && str[i] != ')') {
              if (str[i] == '\"')
                break;

              str4 += str[i++];
            }
          }

          while (i < len && str[i] != ')')
            ++i;

          if (i < len && str[i] == ')') {
            ++i;

            str3 = str3.simplified();

            // TODO: title
            if (str4 != "")
              str1 += QString("<img src=\"%1\" title=\"%2\" alt=\"%3\"/>").
                        arg(str3).arg(str4).arg(str2);
            else
              str1 += QString("<img src=\"%1\" alt=\"%2\"/>").arg(str3).arg(str2);
          }
          else {
            i = i1;

            str1 += str[i++];
          }
        }
        else if (i < len && str[i] == '[') {
          ++i;

          while (i < len && str[i] != ']')
            str3 += str[i++];

          if (i < len && str[i] == ']') {
            ++i;

            str1 += QString("<img src=\"%1\" alt=\"%2\"/>").arg(str3).arg(str2);
          }
          else {
            i = i1;

            str1 += str[i++];
          }
        }
        else {
          LinkRef ref;

          if (markdown()->getLink(str2, ref)) {
            if (ref.title != "")
              str1 += QString("<img src=\"%1\" alt=\"%2\" title=\"%3\"/>").
                       arg(ref.dest).arg(ref.ref).arg(ref.title);
            else
              str1 += QString("<img src=\"%1\" alt=\"%2\"/>").
                       arg(ref.dest).arg(ref.ref);
          }
          else {
            i = i1;

            str1 += str[i++];
          }
        }
      }
      else {
        i = i1;

        str1 += str[i++];
      }
    }
    // link
    else if (str[i] == '[') {
      int i1 = i;

      ++i;

      QString str2;

      while (i < len && str[i] != ']')
        str2 += str[i++];

      if (i < len && str[i] == ']') {
        ++i;

        QString str3;

        if      (i < len && str[i] == '(') {
          ++i;

          while (i < len && str[i] != ')')
            str3 += str[i++];

          if (i < len && str[i] == ')') {
            ++i;

            str1 += QString("<a href=\"%1\">%2</a>").arg(str3).arg(str2);
          }
          else {
            i = i1;

            str1 += str[i++];
          }
        }
        else if (i < len && str[i] == '[') {
          ++i;

          while (i < len && str[i] != ']')
            str3 += str[i++];

          if (i < len && str[i] == ']') {
            ++i;

            str1 += QString("<a href=\"%1\">%2</a>").arg(str3).arg(str2);
          }
          else {
            i = i1;

            str1 += str[i++];
          }
        }
        else {
          LinkRef ref;

          if (markdown()->getLink(str2, ref)) {
            if (ref.title != "")
              str1 += QString("<a href=\"%1\" title=\"%2\">%3</a>").
                       arg(ref.ref).arg(ref.title).arg(ref.dest);
            else
              str1 += QString("<a href=\"%1\">%2</a>").arg(ref.ref).arg(ref.dest);
          }
          else {
            i = i1;

            str1 += str[i++];
          }
        }
      }
      else {
        i = i1;

        str1 += str[i++];
      }
    }
    // TODO: auto links

    // escape special chars
    else if (str[i] == '<') {
      QString ref;

      if (isAutoLink(str, i, ref)) {
        QString ref1 = replaceEmbeddedStyles(ref);

        str1 += QString("<a href=\"%1\">%2</a>").arg(ref).arg(ref1);
      }
      else {
        str1 += "&lt;"; ++i;
      }
    }
    else if (str[i] == '>') {
      str1 += "&gt;"; ++i;
    }
    else if (str[i] == '"') {
      str1 += "&quot;"; ++i;
    }
    else if (str[i] == '&') {
      str1 += "&amp;"; ++i;
    }
    else
      str1 += str[i++];
  }

  return str1;
}

// check for char surrounded text
int
CMarkdownBlock::
parseSurroundText(const QString &str, int &i, const QChar &c, QString &str2) const
{
  int len = str.length();

  if (i >= len || str[i] != c)
    return 0;

  int  i1    = i;
  bool found = false;

  int nc = 1;

  ++i;

  while (i < len && str[i] == c) {
    ++nc; ++i;
  }

  while (i < len) {
    if (i < len && str[i] == c) {
      ++i;

      int nc1 = 1;

      while (i < len && nc1 < nc && str[i] == c) {
        ++nc1; ++i;
      }

      if (nc1 == nc) {
        found = true;
        break;
      }
      else {
        for (int j = 0; j < nc1; ++j)
          str2 += c;
      }
    }
    else
      str2 += str[i++];
  }

  if (! found) {
    i = i1;
    return 0;
  }

  return nc;
}

QString
CMarkdownBlock::
replaceHtmlChars(const QString &str) const
{
  QString str1;

  int i   = 0;
  int len = str.length();

  while (i < len) {
    if      (str[i] == '<') {
      str1 += "&lt;"; ++i;
    }
    else if (str[i] == '>') {
      str1 += "&gt;"; ++i;
    }
    else if (str[i] == '"') {
      str1 += "&quot;"; ++i;
    }
    else if (str[i] == '&') {
      str1 += "&amp;"; ++i;
    }
    else
      str1 += str[i++];
  }

  return str1;
}

bool
CMarkdownBlock::
isAutoLink(const QString &str, int &i, QString &ref) const
{
  int i1 = i;

  int len = str.length();

  if (i1 >= len || str[i1] != '<')
    return false;

  skipSpace(str, i1);

  ++i1;

  QString scheme;

  while (i1 < len && ! str[i1].isSpace() && str[i1] != ':') {
    if (! str[i1].isLetter())
      return false;

    scheme += str[i1++];
  }

  skipSpace(str, i1);

  if (i1 >= len || str[i1] != ':')
    return false;

  ++i1;

  skipSpace(str, i1);

  QString ref1;

  while (i1 < len && str[i1] != '>')
    ref1 += str[i1++];

  if (i1 >= len || str[i1] != '>')
    return false;

  ++i1;

  ref = QString("%1:%2").arg(scheme).arg(ref1);

  //ref = str.mid(i + 1, i1 - i - 2);

  i = i1;

  return true;
}

bool
CMarkdownBlock::
isASCIIPunct(const QChar &c) const
{
  static QString chars("!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~");

  return (chars.indexOf(c) >= 0);
}

int
CMarkdownBlock::
skipSpace(const QString &str, int &i) const
{
  int len = str.length();

  int n = 0;

  while (i < len && str[i].isSpace()) {
    ++i; ++n;
  }

  return n;
}

int
CMarkdownBlock::
backSkipSpace(const QString &str, int &i) const
{
  int n = 0;

  while (i >= 0 && str[i].isSpace()) {
    --i; ++n;
  }

  return n;
}

int
CMarkdownBlock::
skipChar(const QString &str, int &i, const QChar &c) const
{
  int len = str.length();

  int n = 0;

  while (i < len && str[i] == c) {
    ++i; ++n;
  }

  return n;
}

int
CMarkdownBlock::
backSkipChar(const QString &str, int &i, const QChar &c) const
{
  int n = 0;

  while (i >= 0 && str[i] == c) {
    --i; ++n;
  }

  return n;
}

bool
CMarkdownBlock::
getLine(LineData &line)
{
  line.brk = false;

  if (currentLine_ >= int(lines_.size()))
    return false;

  QString str = lines_[currentLine_++].line;

  QString spaces;
  int     ns = 0;

  int len = str.size();

  int i = 0;

  line.line  = "";
  line.blank = true;

  while (i < len && str[i] != '\n') {
    if (str[i].isSpace()) {
      // expand tabs
      if (str[i] == '\t')
        spaces += "    ";
      else
        spaces += " ";

      ++ns;
      ++i;
    }
    else {
      line.line += spaces;

      spaces = "";
      ns     = 0;

      line.blank = false;

      line.line += str[i++];
    }
  }

  if (ns >= 2)
    line.brk = true;

  line.indent = 0;

  skipSpace(line.line, line.indent);

  return true;
}

void
CMarkdownBlock::
ungetLine()
{
  assert(currentLine_ > 0);

  --currentLine_;
}

CMarkdownBlock *
CMarkdownBlock::
startBlock(BlockType type)
{
  CMarkdownBlock *block = new CMarkdownBlock(currentBlock_, type);

  currentBlock_->addBlock(block);

  currentBlock_ = block;

  if (markdown()->isDebug())
    std::cerr << "startBlock " << tagName(type).toStdString() << std::endl;

  return block;
}

void
CMarkdownBlock::
addBlockLine(const QString &line, bool brk)
{
    QString blockLine = line;
    if(blockLine.endsWith("}"))
    {
      int indexOfId = blockLine.lastIndexOf("{#");
      if(indexOfId != -1)
      {
          QString id = blockLine.mid(indexOfId + 2, blockLine.length() - indexOfId - 3);
          currentBlock_->setId(id);
          blockLine.truncate(indexOfId);
      }
    }


  if (markdown()->isDebug())
    std::cerr << "add: " << blockLine.toStdString() << std::endl;

  currentBlock_->addLine(Line(blockLine, brk));
}

void
CMarkdownBlock::
appendBlockLine(const QString &line)
{
  if (markdown()->isDebug())
    std::cerr << "append: " << line.toStdString() << std::endl;

  currentBlock_->appendLine(line);
}

void
CMarkdownBlock::
flushBlocks()
{
  while (currentBlock_ != rootBlock_)
    endBlock();
}

void
CMarkdownBlock::
endBlock()
{
  if (currentBlock_ == rootBlock_)
    return;

  if (markdown()->isDebug())
    std::cerr << "endBlock " << tagName(currentBlock_->blockType()).toStdString() << std::endl;

  currentBlock_ = currentBlock_->parent();
}

void
CMarkdownBlock::
print(int depth) const
{
  QString tag = tagName(type_);

  for (int i = 0; i < depth; ++i)
    std::cout << "  ";

  std::cout << QString("-> %1").arg(tag).toStdString() << std::endl;

  for (auto &l : lines_) {
    for (int i = 0; i < depth; ++i)
      std::cout << "  ";

    std::cout << "  \"" << l.line.toStdString() << "\"" << std::endl;
  }

  for (auto &b : blocks_)
    b->print(depth + 1);
}

QString
CMarkdownBlock::
toHtml() const
{
  CMarkdownBlock *th = const_cast<CMarkdownBlock *>(this);

  th->process();

  //---

  QString html;

  QString tag = tagName(type_);

  bool single = isSingleLineType(type_);

  bool empty = false;

  if (single) {
    if (! processed_)
      empty = (lines_.empty() && blocks_.empty());
    else
      empty = blocks_.empty();
  }

  if (empty) {
    html += id_.isEmpty() ? QString("<%1/>\n").arg(tag) : QString("<%1 id='%2'/>\n").arg(tag).arg(id_);
  }
  else {
    if (single)
      html += id_.isEmpty() ? QString("<%1>").arg(tag) : QString("<%1 id='%2'>").arg(tag).arg(id_);
    else
      html += id_.isEmpty() ? QString("<%1>\n").arg(tag) : QString("<%1 id='%2'>\n").arg(tag).arg(id_);

    if (! processed_) {
      int  nl  = 0;
      bool brk = false;

      QString line1;

      for (auto &line : lines_) {
        if (nl > 0) {
          if (brk)
            line1 += "\t";
          else
            line1 += "\n";
        }

        line1 += line.line;

        brk = line.brk;

        ++nl;
      }

      if (type_ !=  BlockType::CODE)
        line1 = replaceEmbeddedStyles(line1);

      QString line2;

      for (int i = 0; i < line1.size(); ++i) {
        if (line1[i] == '\t')
          line2 += "<br>\n";
        else
          line2 += line1[i];
      }

      html += line2;
    }

    int nb = 0;

    for (auto &b : blocks_) {
      html += b->toHtml();

      ++nb;
    }

    if (single)
      html += QString("</%1>\n").arg(tag);
    else
      html += QString("</%1>\n").arg(tag);
  }

  return html;
}

QString
CMarkdownBlock::
tagName(BlockType type)
{
  if      (type == BlockType::DOCUMENT  ) return "document";
  else if (type == BlockType::P         ) return "p";
  else if (type == BlockType::BLOCKQUOTE) return "blockquote";
  else if (type == BlockType::H1        ) return "h1";
  else if (type == BlockType::H2        ) return "h2";
  else if (type == BlockType::H3        ) return "h3";
  else if (type == BlockType::H4        ) return "h4";
  else if (type == BlockType::H5        ) return "h5";
  else if (type == BlockType::H6        ) return "h6";
  else if (type == BlockType::UL        ) return "ul";
  else if (type == BlockType::OL        ) return "ol";
  else if (type == BlockType::LI        ) return "li";
  else if (type == BlockType::PRE       ) return "pre";
  else if (type == BlockType::CODE      ) return "code";
  else if (type == BlockType::TABLE     ) return "table";
  else if (type == BlockType::TR        ) return "tr";
  else if (type == BlockType::TD        ) return "td";
  else if (type == BlockType::HR        ) return "hr";
  else                                    return "??";
}

bool
CMarkdownBlock::
isSingleLineType(BlockType type)
{
  if      (type == BlockType::H1) return true;
  else if (type == BlockType::H2) return true;
  else if (type == BlockType::H3) return true;
  else if (type == BlockType::H4) return true;
  else if (type == BlockType::H5) return true;
  else if (type == BlockType::H6) return true;
  else if (type == BlockType::LI) return true;
  else if (type == BlockType::HR) return true;
  else if (type == BlockType::P ) return true;

  return false;
}

bool
CMarkdownBlock::
isRecurseType(BlockType type)
{
  if      (type == BlockType::DOCUMENT  ) return true;
  else if (type == BlockType::BLOCKQUOTE) return true;
  else if (type == BlockType::LI        ) return true;
  else if (type == BlockType::PRE       ) return true;
  else if (type == BlockType::TABLE     ) return true;

  return false;
}

void
CMarkdownBlock::
setId(const QString &id)
{
    id_ = id;
}

QString
CMarkdownBlock::
id() const
{
    return id_;
}
