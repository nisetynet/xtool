#pragma once

#include <boost/program_options.hpp>
#include <string_view>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

// supports custom error message
class invalid_option_error_with_msg
    : public boost::program_options::validation_error {
public:
  invalid_option_error_with_msg(std::string_view const option_name,
                                std::string_view const original_token,
                                std::string_view const error_message)
      : boost::program_options::validation_error(
            boost::program_options::validation_error::invalid_option_value,
            option_name.data(), original_token.data()),
        m_custom_message(error_message) {}

  virtual const char *what() const noexcept override {
    auto const what = boost::program_options::validation_error::what();

    m_message = fmt::format("{}, got '{}', detail: {}", what,
                            this->m_substitutions.at("original_token"),
                            m_custom_message);
    return m_message.c_str();
  }

private:
  std::string m_custom_message;
};

[[nodiscard]] boost::program_options::variables_map
parse_program_options(int const argc, char **const argv);