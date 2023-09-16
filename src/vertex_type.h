
namespace TANG
{
	struct VertexType
	{
		VertexType() : pos(0.0f, 0.0f, 0.0f), normal(0.0f, 0.0f, 0.0f), uv(0.0f, 0.0f)
		{
		}
		~VertexType() { }
		VertexType(const VertexType& other)
		{
			pos = other.pos;
			normal = other.normal;
			uv = other.uv;
		}

		// Utility functions / operator overloads
		bool operator==(const VertexType& other) const
		{
			return pos == other.pos && normal == other.normal && uv == other.uv;
		}

		// Member variables
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 uv;
	};

}