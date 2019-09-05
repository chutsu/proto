# Linear Triangulation

There are various methods for triangulating a 3D point obeserved from at least
two camera views. The linear triangulation method [Hartley2003] is
frequently used. This method assumes a pair of homogeneous pixel measurements
$\Vec{z}$ and $\Vec{z}' \in \real^{3}$ that observes the same 3D
point, $\Vec{X} \in \real^{4}$, in homogeneous coordinates from two different
camera frames. The homogeneous projection from 3D to 2D with a known camera
matrix $\Mat{P} \in \real^{3 \times 4}$ for each measurement is given as,
\begin{align}
	\begin{split}
		\Vec{z} &= \mathbf{P} \mathbf{X} \\
		\Vec{z}' &= \mathbf{P}' \mathbf{X}.
	\end{split}
\end{align}
These equations can be combined to form a system of equations of the form
$\Mat{A} \Vec{x} = \Vec{0}$. To eliminate the homogeneous scale factor we apply
a cross product to give three equations for each image point, for example
$\Vec{z} \times (\Mat{P} \Mat{X}) = \Vec{0}$ writing this out gives
\begin{align}
	\begin{split}
		x (\Vec{p}^{3T} \Vec{X}) - (\Vec{p}^{1T} \Vec{X}) = 0 \\
		y (\Vec{p}^{3T} \Vec{X}) - (\Vec{p}^{2T} \Vec{X}) = 0 \\
		x (\Vec{p}^{2T} \Vec{X}) - y (\Vec{p}^{1T} \Vec{X}) = 0
	\end{split}
	\label{eq:derivation}
\end{align}
where $\Vec{p}^{iT}$ is the $i^{\mbox{th}}$ row of $\Vec{P}$.

From \eqref{eq:derivation}, an equation of the form
$\Mat{A} \Vec{x} = \Vec{0}$ for each image point can be formed, where
$\Vec{x}$ represents the unknown homogeneous feature location to be
estimated, and $\Mat{A}$ is given as
\begin{align}
  \mathbf{A} =
  \begin{bmatrix}
    x (\Vec{p}^{3T}) - (\Vec{p}^{1T}) \\
    y (\Vec{p}^{3T}) - (\Vec{p}^{2T}) \\
    x' (\Vec{p'}^{3T}) - (\Vec{p'}^{1T}) \\
    y' (\Vec{p'}^{3T}) - (\Vec{p'}^{2T})
  \end{bmatrix}
  \label{eq:linear_triangulation_ derivation}
\end{align}
giving a total of four equations in four homogeneous unknowns. Solving for
$\Vec{A}$ using SVD allows us to estimate the initial feature location.

In an ideal world, the position of 3D points can be solved as a system of
equations using the linear triangulation method. In reality, however, errors
are present in the camera poses and pixel measurements. The pixel measurements
observing the same 3D point are generally noisy. In addition, the camera models
and distortion models used often do not model the camera projection or
distortion observed perfectly. Therefore to obtain the best results an
iterative method should be used. This problem is generally formulated as a
non-linear least square problem and can be solved by numerical methods, such as
the Gauss-Newton algorithm.



### References:

[Hartley2003]: Hartley, Richard, and Andrew Zisserman. Multiple view geometry
in computer vision. Cambridge university press, 2003.