#include "denoiser.h"

Denoiser::Denoiser() : m_useTemportal(false) {}

void Denoiser::Reprojection(const FrameInfo &frameInfo) {
    int height = m_accColor.m_height;
    int width = m_accColor.m_width;
    Matrix4x4 preWorldToScreen =
        m_preFrameInfo.m_matrix[m_preFrameInfo.m_matrix.size() - 1];
    Matrix4x4 preWorldToCamera =
        m_preFrameInfo.m_matrix[m_preFrameInfo.m_matrix.size() - 2];
#pragma omp parallel for
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // TODO: Reproject
            int cur_id = frameInfo.m_id(x, y);
            if (cur_id == -1) {
                m_valid(x, y) = false;
                m_misc(x, y) = Float3(0.f);
                continue;
            }
            Matrix4x4 model_inverse_matrix = Inverse(frameInfo.m_matrix[cur_id]);
            Float3 obj_pos =
                model_inverse_matrix(frameInfo.m_position(x, y), Float3::Point);
            Float3 pre_screen_pos =
                preWorldToScreen(m_preFrameInfo.m_matrix[cur_id](obj_pos, Float3::Point), Float3::Point);
            pre_screen_pos.x = std::min((int)pre_screen_pos.x, width - 1);
            pre_screen_pos.x = std::max((int)pre_screen_pos.x, 0);
            pre_screen_pos.y = std::min((int)pre_screen_pos.y, height - 1);
            pre_screen_pos.y = std::max((int)pre_screen_pos.y, 0);
            int pre_id = m_preFrameInfo.m_id(pre_screen_pos.x, pre_screen_pos.y);
            if (pre_id != cur_id) {
                m_valid(x, y) = false;
                m_misc(x, y) = Float3(0.f);
                continue;
            }

			m_valid(x, y) = true;
            m_misc(x, y) = m_accColor(pre_screen_pos.x, pre_screen_pos.y);
        }
    }
    std::swap(m_misc, m_accColor);
}

void Denoiser::TemporalAccumulation(const Buffer2D<Float3> &curFilteredColor) {
    int height = m_accColor.m_height;
    int width = m_accColor.m_width;
    int kernelRadius = 3;
#pragma omp parallel for
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // TODO: Temporal clamp
            Float3 color = m_accColor(x, y);
            // TODO: Exponential moving average
            float alpha = 1.0f;
            if (m_valid(x, y)) {
                int x_start = std::max(0, x - kernelRadius);
                int x_end = std::min(width - 1, x + kernelRadius);
                int y_start = std::max(0, y - kernelRadius);
                int y_end = std::min(height - 1, y + kernelRadius);
                int cnt = (x_end - x_start + 1) * (y_end - y_start + 1);
                Float3 mu(0.0f);
                Float3 sigma(0.0f);
                for (int i = x_start; i <= x_end; ++i) {
                    for (int j = y_start; j <= y_end; ++j) {
                        mu += curFilteredColor(i, j);
                        sigma += Sqr(curFilteredColor(x, y) - curFilteredColor(i, j));
					}
				}
                mu /= float(cnt);
                sigma = SafeSqrt(sigma / float(cnt));
                color = Clamp(color, mu - sigma * m_colorBoxK, mu + sigma * m_colorBoxK);
                alpha = m_alpha;
			}
            m_misc(x, y) = Lerp(color, curFilteredColor(x, y), alpha);
        }
    }
    std::swap(m_misc, m_accColor);
}

Buffer2D<Float3> Denoiser::Filter(const FrameInfo &frameInfo) {
    int height = frameInfo.m_beauty.m_height;
    int width = frameInfo.m_beauty.m_width;
    Buffer2D<Float3> filteredImage = CreateBuffer2D<Float3>(width, height);
    int kernelRadius = 16;
#pragma omp parallel for
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // TODO: Joint bilateral filter
            // filteredImage(x, y) = frameInfo.m_beauty(x, y);
            int x_start = std::max(0, x - kernelRadius);
            int x_end = std::min(width - 1, x + kernelRadius);
            int y_start = std::max(0, y - kernelRadius);
            int y_end = std::min(height - 1, y + kernelRadius);
            float weight_sum = 0.0f;
            for (int i = x_start; i <= x_end; ++i) {
                for (int j = y_start; j <= y_end; ++j) {
                    float w_distance = SqrDistance(frameInfo.m_position(i, j),
                                                    frameInfo.m_position(x, y));
                    w_distance /= (-2 * m_sigmaCoord * m_sigmaCoord);
                    float w_color =
                        SqrDistance(frameInfo.m_beauty(i, j), frameInfo.m_beauty(x, y));
                    w_color /= (-2 * m_sigmaColor * m_sigmaColor);

					float w_normal =
                        SafeAcos(Dot(frameInfo.m_normal(i, j), frameInfo.m_normal(x, y)));
                    w_normal *= w_normal;
                    w_normal /= (-2 * m_sigmaNormal * m_sigmaNormal);

					float w_plane = 0.0;
                    float dist =
                        Distance(frameInfo.m_position(x, y), frameInfo.m_position(i, j));
                    if (dist > 0) {
                        w_plane = std::max(0.0f, Dot(frameInfo.m_normal(x, y),
                                                    (frameInfo.m_position(i, j) -
                                                     frameInfo.m_position(x, y)) /
                                                        dist));
					}
                    w_plane /= (-2 * m_sigmaPlane * m_sigmaPlane);

					float weight = exp(w_distance + w_color + w_normal + w_plane);
                    weight_sum += weight;
                    filteredImage(x, y) += frameInfo.m_beauty(i, j) * weight;
				}
			}

			if (weight_sum == 0.0f)
				filteredImage(x, y) = frameInfo.m_beauty(x, y);
			else
				filteredImage(x, y) /= weight_sum;
        }
    }
    return filteredImage;
}

void Denoiser::Init(const FrameInfo &frameInfo, const Buffer2D<Float3> &filteredColor) {
    m_accColor.Copy(filteredColor);
    int height = m_accColor.m_height;
    int width = m_accColor.m_width;
    m_misc = CreateBuffer2D<Float3>(width, height);
    m_valid = CreateBuffer2D<bool>(width, height);
}

void Denoiser::Maintain(const FrameInfo &frameInfo) { m_preFrameInfo = frameInfo; }

Buffer2D<Float3> Denoiser::ProcessFrame(const FrameInfo &frameInfo) {
    // Filter current frame
    Buffer2D<Float3> filteredColor;
    filteredColor = Filter(frameInfo);

    // Reproject previous frame color to current
    if (m_useTemportal) {
        Reprojection(frameInfo);
        TemporalAccumulation(filteredColor);
    } else {
        Init(frameInfo, filteredColor);
    }

    // Maintain
    Maintain(frameInfo);
    if (!m_useTemportal) {
        m_useTemportal = true;
    }
    return m_accColor;
}
