#include "Parser.hpp"
#include "AST.hpp"
#include "Logger.hpp"
#include <vector>
#include <print>
#include <cstdlib>

namespace TOML
{
  Parser::Parser
  (
    std::string_view source_view,
    std::string_view filename
  ) noexcept : Sv_SourceView_(source_view), Sv_FileName_(filename)
  {
    TOML::TableNode RootTable
    {
      .name = "",
      .FirstChildIndx = NodeIdx::None,
      .IsArrayOfTable = false,
      ._pad = {}
    };

    this->RootTableIdx = this->Arena_.EmplaceNode(std::move(RootTable));
    this->LastTableIdx = this->RootTableIdx;
  }

  auto Parser::Peek() const noexcept -> Token::TokenType
  {
    if (this->Cursor_ >= this->Toks_.size())
      return Token::TokenType::EndOfFile;

    return this->Toks_[this->Cursor_].Type;
  }

  auto Parser::Consume() noexcept
  -> const Token::TokenData&
  {
    return this->Toks_[this->Cursor_++];
  }

  auto Parser::StartOfStatement() const noexcept -> bool
  {
    if (this->Cursor_ >= this->Toks_.size())
      return true;

    const auto Type = this->Toks_[this->Cursor_].Type;

    if
    (
      Type == Token::TokenType::LeftBracket ||
      Type == Token::TokenType::EndOfFile
    ) return true;

    if (Type == Token::TokenType::Identifier)
      if (this->Cursor_ + 1 < this->Toks_.size())
        return this->Toks_[this->Cursor_ + 1].Type == Token::TokenType::Assign;

    return false;
  }

  auto Parser::ReportError
  (
    const Token::TokenData &Tokens,
    std::string_view Msg
  ) const noexcept -> void
  {
    std::println
    (
      stderr,
      "\033[1m{}:{}:{}:\033[0m {}{}: {}{}\033[0m",
      this->Sv_FileName_,
      Tokens.Lexer_Size_t_Line_,
      Tokens.Lexer_Size_t_Column_,
      ConsoleColor::BOLD_RED,
      "error",
      ConsoleColor::RESET,
      Msg
    );

    if
    (
      Tokens.Lexer_Size_t_Line_ == 0 ||
      this->Sv_SourceView_.empty()
    ) std::exit(1);

    size_t st_LineStart_ = Tokens.Lexer_Size_t_Offset_;

    while
    (
      st_LineStart_ > 0 &&
      this->Sv_SourceView_[st_LineStart_ - 1] != '\n'
    ) st_LineStart_--;

    size_t st_LineEnd_ = Tokens.Lexer_Size_t_Offset_;

    while
    (
      st_LineEnd_ < this->Sv_SourceView_.size() &&
      this->Sv_SourceView_[st_LineEnd_] != '\n'
    ) st_LineEnd_++;

    std::string_view Sv_SourceLine_ = this->Sv_SourceView_.substr
    (st_LineStart_,
      st_LineEnd_ - st_LineStart_
    );

    std::println(stderr, "{:<5} | {}", Tokens.Lexer_Size_t_Line_, Sv_SourceLine_);
    const size_t kSt_PadLen_ = (Tokens.Lexer_Size_t_Column_ > 0) ? (Tokens.Lexer_Size_t_Column_ - 1 ) : 0;

    std::string Pad(kSt_PadLen_, ' ');

    std::string Underline = "^";

    if (Tokens.Sv_Val_.length() > 1)
      Underline.append(Tokens.Sv_Val_.length() - 1, '~');

    std::println
    (
      stderr,
      "      | {}{}{}{}",
      Pad,
      ConsoleColor::BOLD_RED,
      Underline,
      ConsoleColor::RESET
    );
    std::exit(1);
  }

  auto Parser::ParseTable() noexcept -> void
  {
    this->Consume();
    bool IsArrayOfTable = false;


    if (this->Peek() == Token::TokenType::LeftBracket)
    {
      this->Consume();
      IsArrayOfTable = true;
    }

    const auto& FirstTok = this->Toks_[this->Cursor_];
    size_t StartOffset = FirstTok.Lexer_Size_t_Offset_;
    size_t EndOffset = StartOffset;

    while
    (
      this->Peek() != Token::TokenType::RightBracket &&
      this->Peek() != Token::TokenType::EndOfFile
    )
    {
      const auto& Tok = this->Consume();
      EndOffset = Tok.Lexer_Size_t_Offset_ + Tok.Sv_Val_.length();
    }

    std::string_view ka_NameTok = this->Sv_SourceView_.substr(StartOffset, EndOffset - StartOffset);

    if (this->Peek() != Token::TokenType::RightBracket)
      this->ReportError(this->Toks_[this->Cursor_], "Expected closing ']' for group");

    this->Consume();

    if (IsArrayOfTable)
    {
      if (this->Peek() != Token::TokenType::RightBracket)
        this->ReportError(this->Toks_[this->Cursor_], "Expected second closing ']' for array of tables.");

      this->Consume();
    }

    TableNode TablePayLoad
    {
      .name = ka_NameTok,
      .FirstChildIndx = NodeIdx::None,
      .IsArrayOfTable = IsArrayOfTable,
      ._pad = {}
    };

    const auto ka_NewTableIdx_ = this->Arena_.EmplaceNode(std::move(TablePayLoad));

    if (this->RootTableIdx == NodeIdx::None)
      this->RootTableIdx = ka_NewTableIdx_;

    else [[
      /* nullAttr*/
    ]] this->Arena_.GetNode(this->LastTableIdx).NextSiblingIndx = ka_NewTableIdx_;

    this->LastTableIdx = ka_NewTableIdx_;
  }

  auto Parser::ParseKeyValue() noexcept -> void
  {
    if (this->LastTableIdx == NodeIdx::None)
      this->ReportError(this->Toks_[this->Cursor_], "Key-value pair declared outside of a group block.");

    const auto& KeyToken = this->Consume();

    if (this->Peek() != Token::TokenType::Assign)
      this->ReportError(this->Toks_[this->Cursor_], "Expected '=' operator after key identifier");

    this->Consume();

    if (this->Peek() == Token::TokenType::LeftBracket)
    {
      this->Consume();

      KeyValueNode KeyPayload
      {
        .Key = KeyToken.Sv_Val_,
        .Value = "",
        .TypeDisc = 4,
        ._pad = {}
      };

      const auto KeyValueIdx = this->Arena_.EmplaceNode(std::move(KeyPayload));
      auto& A_TableNode = this->Arena_.GetNode(this->LastTableIdx);
      auto& TableData = std::get<TableNode>(A_TableNode.Payload);

      if (TableData.FirstChildIndx == NodeIdx::None)
        TableData.FirstChildIndx = KeyValueIdx;

      else
      {
        auto Current = TableData.FirstChildIndx;

        while (this->Arena_.GetNode(Current).NextSiblingIndx != NodeIdx::None)
          Current = this->Arena_.GetNode(Current).NextSiblingIndx;

        this->Arena_.GetNode(Current).NextSiblingIndx = KeyValueIdx;
      }

      ArrayNode ArrayPayload { .FirstChildIndx = NodeIdx::None };
      const auto ArrayIndx = this->Arena_.EmplaceNode(std::move(ArrayPayload));
      this->Arena_.GetNode(KeyValueIdx).NextSiblingIndx = ArrayIndx;

      NodeIdx LastElementIdx = NodeIdx::None;


      while
      (
        this->Peek() != Token::TokenType::RightBracket &&
        this->Peek() != Token::TokenType::EndOfFile
      )
      {
        const auto& ElementToken  = this->Consume();
        KeyValueNode ElementPayload
        {
          .Key = "",
          .Value = ElementToken.Sv_Val_,
          .TypeDisc = static_cast<std::uint32_t>(ElementToken.Type),
          ._pad = {}
        };

        const auto ElementIdx = this->Arena_.EmplaceNode(std::move(ElementPayload));
        auto& A_ArrayNode = this->Arena_.GetNode(ArrayIndx);
        auto& ArrayData = std::get<ArrayNode>(A_ArrayNode.Payload);

        if (ArrayData.FirstChildIndx == NodeIdx::None)
          ArrayData.FirstChildIndx = ElementIdx;

        else [[
          /*nullAttr*/
        ]] this->Arena_.GetNode(LastElementIdx).NextSiblingIndx = ElementIdx;

        LastElementIdx = ElementIdx;

        if
        (
          this->Peek() == Token::TokenType::Comma
        )
        {
          this->Consume();

          if (this->Peek() == Token::TokenType::RightBracket)
            this->ReportError(this->Toks_[this->Cursor_], "Expected another array element but got ']'");
        }
        else if
        (
          this->Peek() == Token::TokenType::RightBracket
        ) break;
        else [[
          /* nullAttr */
        ]] this->ReportError(this->Toks_[this->Cursor_], "Expected ',' or ']' after array elements.");
      }

      if (this->Peek() != Token::TokenType::RightBracket)
        this->ReportError(this->Toks_[this->Cursor_], "Unterminated array block, missing closing ']'");

      this->Consume();


    } else
    {
      const auto& StartValueToken = this->Toks_[this->Cursor_];

      if
      (
        StartValueToken.Type == Token::TokenType::EndOfFile ||
        this->StartOfStatement()
      ) this->ReportError(StartValueToken, "Missing value assignment after '='");

      const size_t StartOffset = StartValueToken.Lexer_Size_t_Offset_;
      size_t EndOffset = StartOffset + StartValueToken.Sv_Val_.length();


      while
      (
        !this->StartOfStatement() &&
        this->Peek() != Token::TokenType::EndOfFile
      )
      {
        const auto& ka_ContentToken_ = this->Consume();
        EndOffset = ka_ContentToken_.Lexer_Size_t_Offset_ + ka_ContentToken_.Sv_Val_.length();
      }

      const auto CombinedLength = EndOffset - StartOffset;

      std::string_view ValueView = this->Sv_SourceView_.substr(StartOffset, CombinedLength);

      KeyValueNode ValuePayload
      {
        .Key = KeyToken.Sv_Val_,
        .Value = ValueView,
        .TypeDisc = static_cast<std::uint32_t>(StartValueToken.Type),
        ._pad = {}
      };

      const auto KeyValueIdx = this->Arena_.EmplaceNode(std::move(ValuePayload));

      auto& A_TableNode = this->Arena_.GetNode(this->LastTableIdx);
      auto& TableData = std::get<TableNode>(A_TableNode.Payload);

      if (TableData.FirstChildIndx == NodeIdx::None)
        TableData.FirstChildIndx = KeyValueIdx;

      else
      {
        auto Current = TableData.FirstChildIndx;
        while
        (
          this->Arena_.GetNode(Current).NextSiblingIndx != NodeIdx::None
        ) Current = this->Arena_.GetNode(Current).NextSiblingIndx;

        this->Arena_.GetNode(Current).NextSiblingIndx = KeyValueIdx;
      }
    }
  }


  auto Parser::ReceiveToken(const Token::TokenData& Token) noexcept -> void
  {
    if
    (
      Token.Type != Token::TokenType::Unknown
    ) this->Toks_.emplace_back(Token);

    else [[
      /* nullAttr */
    ]] this->ReportError(Token, "Encountered unrecognized Lexer Symbol Context");
  }

  auto Parser::Parse() noexcept -> NodeIdx
  {
    while
    (
      this->Cursor_ < this->Toks_.size() &&
      this->Toks_[this->Cursor_].Type != Token::TokenType::EndOfFile
    )
    {
      if (this->Peek() == Token::TokenType::LeftBracket)
        this->ParseTable();
      else if (this->Peek() == Token::TokenType::Identifier)
        this->ParseKeyValue();
      else [[
        /* nullAttr */
      ]] this->ReportError(this->Toks_[this->Cursor_], "Unexpected structural token sequence");
    }

    return this->RootTableIdx;
  }

  auto Parser::GetArena() const noexcept -> const ASTArena&
  {
    return this->Arena_;
  }
}

