#include "stdafx.h"


// GLM は（GLSL ライクな実装のため）個人的にはあまり好かないが、
// double 対応のグラフィックス向け算術ライブラリが含まれているのは GPGPU 連携的な観点からも有用と思われる。
// D3DXMath や XNAMath/DirectXMath はいずれも double に対応していない。
// Direct3D/DirectCompute ではそもそも倍精度対応自体がおまけ扱いなのだが……


// GLM 0.9.2.4 の func_common.hpp, func_common.inl, func_interger.hpp には、
// 西ヨーロッパ言語（Windows-1252）のマルチバイト文字（UCS-2 の 2013H に相当するハイフン）が
// 含まれているので、ASCII の 2DH = '-' に置換しておくこと。
// GLM 0.9.4.3 など、新しいバージョンでは修正されている模様。
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace
{
	// GLM 0.9.2.4 には gluLookAt() 関数に相当するものはデフォルトで用意されていない。
	// 0.9.4.3 のような新しいバージョンには glm::lookAt() として実装されている模様。
	inline glm::mat4x4 BuildMyLookAtMatrix(
		const glm::vec3& eye,
		const glm::vec3& target,
		const glm::vec3& up)
	{
		const glm::vec3 f(glm::normalize(target - eye));
		const glm::vec3 s(glm::normalize(glm::cross(f, glm::normalize(up))));
		const glm::vec3 u(glm::normalize(glm::cross(s, f)));

		const glm::mat4x4 m(
			s.x, s.y, s.z, 0,
			u.x, u.y, u.z, 0,
			-f.x, -f.y, -f.z, 0,
			0, 0, 0, 1);

		return glm::translate<float>(glm::inverse(m), -eye);
	}

	inline glm::mat4x4 BuildMyLookAtMatrix(
		float eyeX, float eyeY, float eyeZ,
		float centerX, float centerY, float centerZ,
		float upX, float upY, float upZ)
	{
		return BuildMyLookAtMatrix(
			glm::vec3(eyeX, eyeY, eyeZ),
			glm::vec3(centerX, centerY, centerZ),
			glm::vec3(upX, upY, upZ));
	}

	// GLM の行列クラスはデフォルト コンストラクタにより単位行列となる。
	const glm::mat4x4 IdentityMatrix(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);



	int DoMyGlmTest1()
	{
		// D3DX Math, XNA Math, DirectXMath 系とは違い、Degree で角度を与えたり、
		// X/Y/Z ごとの回転行列生成メソッドがなく、回転軸ベクトルを指定するメソッドしかなかったりするところは、
		// 固定機能時代の OpenGL 関数の慣習を引きずっているようで個人的にはあまり好ましくない。
		// （それはそうと角度の引数シンボル名は単位が分かるようにしろよ……）
		const auto mA = glm::rotate(glm::mat4(), 90.0f, glm::vec3(1, 0, 0));
		const auto mB = glm::rotate(glm::mat4(), 45.0f, glm::vec3(0, 1, 0));
		const auto mC = mB * mA; // A, B の順で適用。
		const glm::quat qA(mA);
		const glm::quat qB(mB);
		const glm::quat qC(mC);
		const auto qC1 = qB * qA; // A, B の順で適用。
		const auto qC2 = glm::cross(qB, qA); // A, B の順で適用。
		_ASSERTE(qC1 == qC2);

		return 0;
	}

	const int g_DoMyGlmTest1 = DoMyGlmTest1();
}
