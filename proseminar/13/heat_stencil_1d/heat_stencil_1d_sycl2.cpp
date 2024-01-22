#include <CL/sycl.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>

constexpr std::size_t output_resolution = 128;

using Datatype = double;
using Domain = std::vector<Datatype>;

namespace sycl = cl::sycl;

void printTemperature(const Domain &domain);
int verifyTemperature(const Domain &domain);

int main(int argc, char **argv) {
    std::size_t size_domain = 2048;
    std::size_t timesteps = 50000;
    if(argc > 1) {
        size_domain = std::stoul(argv[1]);
        if(argc > 2) {
            timesteps = std::stoul(argv[2]);
        }
    }

    std::cout << "Computing heat-distribution for room size N=" << size_domain << " for T=" << timesteps << std::endl;

    Domain domain_a(size_domain, 273.0);
    const std::size_t source_x = size_domain / 2;

    std::cout << "Heat Source is at " << source_x << std::endl;

    domain_a[source_x] = 273 + 60;

    std::cout << "Initial:\t";
    printTemperature(domain_a);
    std::cout << std::endl;

    Domain domain_b(size_domain, 273.0);

    sycl::queue queue(sycl::cpu_selector{});

    sycl::buffer<Datatype, 1> buffer_a(domain_a.data(), sycl::range<1>(size_domain));
    sycl::buffer<Datatype, 1> buffer_b(domain_b.data(), sycl::range<1>(size_domain));

    auto time_start = std::chrono::system_clock::now();

    for (std::size_t t = 0; t < timesteps; t++) {
        queue.submit([&](sycl::handler &handler) {
            auto acc_a = buffer_a.get_access<sycl::access::mode::read>(handler);
            auto acc_b = buffer_b.get_access<sycl::access::mode::write>(handler);

            handler.parallel_for(sycl::range<1>(size_domain), [=](sycl::id<1> x_id) {
                std::size_t x = x_id[0];
                if (x > 0 && x < size_domain - 1 && x != source_x) {
                    Datatype value_left = acc_a[x - 1];
                    Datatype value_center = acc_a[x];
                    Datatype value_right = acc_a[x + 1];
                    acc_b[x] = value_center + 0.2 * (value_left + value_right + (-2.0 * value_center));
                } else {
                    acc_b[x] = acc_a[x];
                }
            });
        });
        std::swap(buffer_a, buffer_b);

      if (t % 10000 == 0) {

        Domain domain_copy(size_domain);

        // The following would work as well and is simppler but since the exercise
        // explicitly asks for a second kernel, we do not do it this way.
        // This would worke because sycl internally ensure the synchronization
        // without needing to call wait
        // {
        //     auto acc = buffer_a.get_access<sycl::access::mode::read>();
        //     std::copy(acc.get_pointer(), acc.get_pointer() + size_domain, domain_copy.begin());
        // }

        sycl::buffer<Datatype, 1> host_buffer(domain_copy.data(), sycl::range<1>(size_domain));

        queue.submit([&](sycl::handler &cgh) {
            auto acc_device = buffer_a.get_access<sycl::access::mode::read>(cgh);
            auto acc_host = host_buffer.get_access<sycl::access::mode::write>(cgh);

            cgh.parallel_for(sycl::range<1>(size_domain), [=](sycl::id<1> idx) {
                acc_host[idx] = acc_device[idx];
                });
            }).wait();

        std::cout << "Step t=" << t << "\t";
        printTemperature(domain_copy);
        std::cout << std::endl;
      }
    }

    queue.wait_and_throw();

    auto time_end = std::chrono::system_clock::now();
    const std::chrono::duration<double> elapsed_seconds = time_end - time_start;

    {
        auto acc_a = buffer_a.get_access<sycl::access::mode::read>();
        std::copy(acc_a.get_pointer(), acc_a.get_pointer() + size_domain, domain_a.begin());
    }

    std::cout << "\t\t";
    printTemperature(domain_a);
    std::cout << std::endl;

    int verification_result = verifyTemperature(domain_a);
    std::cout << "Computation took " << elapsed_seconds.count() << " seconds" << std::endl;

    return verification_result;
}

void printTemperature(const Domain &domain) {
        const std::string colors = " .-:=+*^X#%@";

        constexpr Datatype max = 273 + 30;
        constexpr Datatype min = 273 + 0;

        // step size in each dimension
        const std::size_t step_size = domain.size() / output_resolution;

        // left border
        std::cout << "X";

        for (std::size_t i = 0; i < output_resolution; i++) {

                // get max temperature in this tile
                Datatype cur_max = 0;
                for (std::size_t x = step_size * i; x < step_size * i + step_size; x++) {
                        cur_max = (cur_max < domain[x]) ? domain[x] : cur_max;
                }
                Datatype temp = cur_max;

                // pick the 'color'
                int c = ((temp - min) / (max - min)) * colors.length();
                c = (c >= static_cast<int>(colors.length())) ? colors.length() - 1 : ((c < 0) ? 0 : c);

                // print the average temperature
                std::cout << colors[c];
        }

        // right border
        std::cout << "X";
}

int verifyTemperature(const Domain &domain) {
        for (std::size_t x = 0; x < domain.size(); x++) {
                if (domain[x] < 273.0 || domain[x] > 273.0 + 60) {
                        std::cout << "Verification failed, grid[" << x << "]=" << domain[x] << std::endl;
                        return EXIT_FAILURE;
                }
        }
        std::cout << "Verification succeeded" << std::endl;
        return EXIT_SUCCESS;
}
