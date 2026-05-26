# Hydrogen Orbitals Visualizer

A quantum mechanics visualizer written in C. It computes and renders 2D cross-sections of hydrogen electron orbital probability densities in real-time using SDL2.

![Demo Visual](demo.mp4)

---

## How It Works

The simulation visualizes the wave functions ($\Psi$) of a single electron bound to a hydrogen nucleus. These are exact solutions to the **time-independent Schrödinger equation** for a Coulomb potential:

$$
-\frac{\hbar^2}{2\mu}\nabla^2 \Psi + V(r)\Psi = E\Psi
$$

$$
V(r) = -\frac{e^2}{4\pi\varepsilon_0 r}
$$

where $\mu \approx m_e$ is the reduced mass (electron mass in the hydrogen approximation), $r$ is the distance from the nucleus, and $E < 0$ for bound states. The code works in **atomic units** ($\hbar = m_e = e = 4\pi\varepsilon_0 = 1$), with the Bohr radius set to $a_0 = 1$.

According to the **Born rule**, the probability of finding the electron in an infinitesimal volume element $d^3r$ is $|\Psi|^2 \, d^3r$. The visualizer maps this probability density directly to pixel brightness.

### Quantum Numbers

Each bound state is labeled by three integers that arise from separating the Schrödinger equation in spherical coordinates $(r, \theta, \phi)$:

| Symbol | Name | Constraint | Physical meaning |
|--------|------|------------|------------------|
| $n$ | Principal | $n \geq 1$ | Energy level and overall spatial extent |
| $l$ | Azimuthal | $0 \leq l < n$ | Orbital angular momentum magnitude |
| $m$ | Magnetic | $-l \leq m \leq l$ | $z$-component of angular momentum |

In atomic units, the energy of a bound state depends only on $n$:

$$
E_n = -\frac{1}{2n^2}
$$

The angular momentum operators give:

$$
L = \sqrt{l(l+1)}\,\hbar, \qquad L_z = m\hbar
$$

The azimuthal number also determines the familiar orbital letter labels:

| $l$ | Label | Shape (informal) |
|-----|-------|------------------|
| 0 | s | Spherically symmetric |
| 1 | p | Two-lobed (dumbbell) |
| 2 | d | Four-lobed / cloverleaf |
| 3 | f | Complex multi-lobed |

For a given $n$, there are $n$ possible values of $l$, and for each $l$ there are $(2l + 1)$ values of $m$, giving **$n^2$ distinct orbitals** per shell. The demo cycles through all valid $(n, l, m)$ combinations from $(1,0,0)$ up to $n = 4$.

---

## The Physics: Separation of Variables

Because the Coulomb potential is spherically symmetric, the wave function factors into a radial part and an angular part:

$$
\Psi_{n,l,m}(r, \theta, \phi) = R_{n,l}(r)\, Y_{l}^{m}(\theta, \phi)
$$

This separation is exact and reduces the three-dimensional PDE into two ordinary differential equations — one in $r$, one in $(\theta, \phi)$.

### Radial Wave Function $R_{n,l}(r)$

The radial equation describes how the electron's amplitude varies with distance from the nucleus. Its solutions involve the **generalized (associated) Laguerre polynomials** $L_{k}^{(\alpha)}(x)$, computed iteratively by `compute_laguerre`.

The normalized radial function implemented in `compute_radial_wave_function` is:

$$
R_{n,l}(r) = N_{n,l}\, \exp\!\left(-\frac{\rho}{2}\right)\, \rho^{l}\, L_{n-l-1}^{(2l+1)}(\rho)
$$

with normalization constant and scaled radius:

$$
N_{n,l} = \sqrt{\left(\frac{2}{n a_0}\right)^{3} \frac{(n-l-1)!}{2n\,(n+l)!}}, \qquad \rho = \frac{2r}{n a_0}
$$

Key physical features encoded in this expression:

- **Exponential decay** $\exp(-\rho/2)$: The electron is overwhelmingly likely to be found near the nucleus; the cloud falls off exponentially at large $r$.
- **Power law** $\rho^{l}$: Higher angular momentum states are suppressed at the origin — this is the origin of the "nodal structure" near $r = 0$ for $l > 0$.
- **Laguerre polynomial**: Introduces $(n - l - 1)$ radial nodes (surfaces where $R_{n,l} = 0$), so higher $n$ orbitals have more concentric shells.

The most probable radius scales roughly as $n^2 a_0$, which is why the renderer zooms out proportionally to $n^2$ when displaying higher shells.

### Spherical Harmonics $Y_{l}^{m}(\theta, \phi)$

The angular part $Y_{l}^{m}$ encodes the orbital's directional shape — the lobes, nodes, and symmetry axes. It is built from **associated Legendre polynomials** $P_{l}^{m}(\cos\theta)$, computed by `compute_legendre`, combined with an azimuthal phase factor:

$$
Y_{l}^{m}(\theta, \phi) = N_{l}^{m}\, P_{l}^{|m|}(\cos\theta)\, \Phi_m(\phi)
$$

where the normalization and azimuthal factors are:

$$
N_{l}^{m} = \sqrt{\frac{2l+1}{4\pi}\, \frac{(l-|m|)!}{(l+|m|)!}}
$$

$$
\Phi_m(\phi) =
\begin{cases}
\cos(m\phi) & \text{if } m > 0 \\
1 & \text{if } m = 0 \\
\sin(|m|\phi) & \text{if } m < 0
\end{cases}
$$

The Legendre polynomial $P_{l}^{m}$ determines how the probability varies with polar angle $\theta$ (colatitude from the $z$-axis). The $\phi$-dependence introduces rotational symmetry around $z$ for $m = 0$, and $|m|$-fold rotational structure for $m \neq 0$.

Together, $R_{n,l}$ and $Y_{l}^{m}$ are orthonormal:

$$
\int_{0}^{\infty} r^2 |R_{n,l}|^2 \, dr = 1
$$

$$
\int_{0}^{2\pi} \int_{0}^{\pi} |Y_{l}^{m}|^2 \sin\theta \, d\theta \, d\phi = 1
$$

---

## From Wave Function to Pixel

### Probability Density

For each screen pixel, the code evaluates the probability density in the **$x$–$z$ plane** (a 2D slice through the 3D orbital with $y = 0$):

$$
P(x, z) = \left|\Psi_{n,l,m}(r, \theta, \phi)\right|^2, \qquad r = \sqrt{x^2 + z^2}
$$

This is computed in `compute_orbital_probability` by combining the radial and angular factors and squaring the result. The slice reveals the nodal planes and lobe structure that define each orbital's geometry.

### Visual Mapping

Raw probability values span many orders of magnitude — the core is orders of magnitude brighter than the tail. The renderer applies a nonlinear brightness pipeline to make structure visible:

1. **Dynamic scaling** by $1/n^2$ to compensate for the lower peak density of diffuse higher shells.
2. **Gamma compression** (raise brightness to the power $0.1$, then square) to reveal faint outer lobes without saturating the core.
3. **Threshold cutoff** to suppress numerical noise in the near-zero regions.
4. **Color mapping** with unequal RGB exponents (red dominant, blue suppressed) to produce the warm glow seen in the demo.

---

## Core Mathematical Components

The total wave function is the product of two independently computed factors:

$$
\Psi_{n,l,m}(r, \theta, \phi) = R_{n,l}(r)\, Y_{l}^{m}(\theta, \phi)
$$

| Component | Function | Polynomial engine | Role |
|-----------|----------|-------------------|------|
| Radial $R_{n,l}(r)$ | `compute_radial_wave_function` | `compute_laguerre` | Distance from nucleus; radial nodes |
| Angular $Y_{l}^{m}(\theta, \phi)$ | `compute_spherical_harmonic` | `compute_legendre` | Lobe geometry and orientation |

Both polynomial evaluators use stable **recurrence relations** rather than closed-form factorial expressions, which keeps the computation efficient and numerically well-behaved across the full $(n, l, m)$ range displayed in the demo.

---

## Project Structure

```text
hydrogen-orbitals/
├── src/
│   └── main.c          # Core simulation logic and SDL2 rendering pipeline
├── demo.mp4            # Video demonstration of the visualizer loop
└── README.md           # Documentation
```
