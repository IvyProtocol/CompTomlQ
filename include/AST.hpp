#ifndef AST_HPP
#define AST_HPP

#include <string_view>
#include <cstdint>
#include <variant>
#include <vector>

namespace TOML
{
  enum class NodeIdx : std::uint32_t { None = 0xFFFFFFFF };

  class KeyValueNode
  {
    public:
      std::string_view Key {};
      std::string_view Value {};
      std::uint32_t TypeDisc {}; // TOML type: int, float, bool, string
      std::byte _pad[4]{};
  };

  class TableNode
  {
    public:
      std::string_view name {};
      NodeIdx FirstChildIndx { NodeIdx::None };
      bool IsArrayOfTable { false };
      std::byte _pad[3]{};
  };

  class ArrayNode
  {
    public:
      NodeIdx FirstChildIndx { NodeIdx::None };
  };

  using ASTNodePayload = std::variant<KeyValueNode, TableNode, ArrayNode>;

  class ASTNode
  {
    public:
      ASTNodePayload Payload;
      NodeIdx NextSiblingIndx { NodeIdx::None };
      std::byte _pad[4]{};
  };

  class ASTArena
  {
    private:
      std::vector<ASTNode> Nodes_ {};

    public:
      [[nodiscard]] auto EmplaceNode(ASTNodePayload&& Payload)
      noexcept -> NodeIdx
      {
        const auto idx = static_cast<std::uint32_t>(this->Nodes_.size());
        this->Nodes_.emplace_back(std::move(Payload), NodeIdx::None);
        return static_cast<NodeIdx>(idx);
      }

      [[nodiscard]] auto GetNode(NodeIdx Idx) noexcept -> ASTNode&
      {
        return this->Nodes_[static_cast<std::uint32_t>(Idx)];
      }

      [[nodiscard]] auto GetNode(NodeIdx Idx) const noexcept -> const ASTNode&
      {
        return this->Nodes_[static_cast<std::uint32_t>(Idx)];
      }
  };


}
#endif
