"""
Proto

Contains the following library code useful for prototyping robotic algorithms:

- YAML
- TIME
- PROFILING
- MATHS
- LINEAR ALGEBRA
- GEOMETRY
- LIE
- TRANSFORM
- MATPLOTLIB
- CV
- DATASET
- FILTER
- STATE ESTIMATION
- CALIBRATION
- SIMULATION
- UNITTESTS

"""
import os
import sys
import glob
import math
import time
import copy
import random
import pickle
import json
import io
import struct
import signal
import socket
import base64
import hashlib
from datetime import datetime
from pathlib import Path
from enum import Enum
from dataclasses import dataclass
from collections import namedtuple
from types import FunctionType
from typing import Optional

import cv2
import yaml
import numpy as np
import scipy
import scipy.sparse
import scipy.sparse.linalg
import pandas

import cProfile
from pstats import Stats

###############################################################################
# YAML
###############################################################################


def load_yaml(yaml_path):
  """ Load YAML and return a named tuple """
  assert yaml_path is not None
  assert yaml_path != ""

  # Load yaml_file
  yaml_data = None
  with open(yaml_path, "r") as stream:
    yaml_data = yaml.safe_load(stream)

  # Convert dict to named tuple
  data = json.dumps(yaml_data)  # Python dict to json
  data = json.loads(data,
                    object_hook=lambda d: namedtuple('X', d.keys())
                    (*d.values()))

  return data


###############################################################################
# TIME
###############################################################################


def sec2ts(time_s):
  """ Convert time in seconds to timestamp """
  return int(time_s * 1e9)


def ts2sec(ts):
  """ Convert timestamp to seconds """
  return ts * 1e-9


###############################################################################
# PROFILING
###############################################################################


def profile_start():
  """ Start profile """
  prof = cProfile.Profile()
  prof.enable()
  return prof


def profile_stop(prof, **kwargs):
  """ Stop profile """
  key = kwargs.get('key', 'cumtime')
  N = kwargs.get('N', 10)

  stats = Stats(prof)
  stats.strip_dirs()
  stats.sort_stats(key).print_stats(N)


###############################################################################
# NETWORK
###############################################################################


def http_status_code_string(code):
  """ Convert status code to string """
  status_code_str = {
      100: "100 Continue",
      101: "101 Switching Protocols",
      200: "200 OK",
      201: "201 Created",
      202: "202 Accepted",
      203: "203 Non-Authoritative Information",
      204: "204 No Content",
      205: "205 Reset Content",
      206: "206 Partial Content",
      300: "300 Multiple Choices",
      301: "301 Moved Permanently",
      302: "302 Found",
      303: "303 See Other",
      304: "304 Not Modified",
      305: "305 Use Proxy",
      307: "307 Temporary Redirect",
      400: "400 Bad Request",
      401: "401 Unauthorized",
      402: "402 Payment Required",
      403: "403 Forbidden",
      404: "404 Not Found",
      405: "405 Method Not Allowed",
      406: "406 Not Acceptable",
      407: "407 Proxy Authentication Required",
      408: "408 Request Time-out",
      409: "409 Conflict",
      410: "410 Gone",
      411: "411 Length Required",
      412: "412 Precondition Failed",
      413: "413 Request Entity Too Large",
      414: "414 Request-URI Too Large",
      415: "415 Unsupported Media Type",
      416: "416 Requested range not satisfiable",
      417: "417 Expectation Failed",
      500: "500 Internal Server Error",
      501: "501 Not Implemented",
      502: "502 Bad Gateway",
      503: "503 Service Unavailable",
      504: "504 Gateway Time-out",
      505: "505 HTTP Version not supported"
  }

  return status_code_str[code]


def http_parse_request(msg_str):
  """ Parse HTTP Request """
  # Parse method, path and HTTP protocol
  msg = msg_str.split("\r\n")
  method, path, protocol = msg[0].split(" ")

  # Parse headers
  headers = {}
  for element in msg[1:]:
    kv = element.strip().split(":", 1)
    key = kv[0].strip()
    if len(key) == 0:
      continue
    headers[key] = kv[1].strip()

  return (protocol, method, path, headers)


def http_form_request(method, path, headers, protocol="HTTP/1.1"):
  """ Form HTTP request """
  msg = f"{method} {path} {protocol}"
  msg += "\r\n"

  for hdr, val in headers.items():
    msg += f"{hdr}: {val}"
    msg += "\r\n"

  msg += "\r\n"  # End of message
  return msg


def http_form_response(status_code, headers, protocol="HTTP/1.1"):
  """ Form HTTP request """
  msg = f"{protocol} {status_code}"
  msg += "\r\n"

  for hdr, val in headers.items():
    msg += f"{hdr}: {val}"
    msg += "\r\n"

  # End of message
  msg += "\r\n"
  return msg


def websocket_hash(ws_key):
  """
  This hashing function:
  1. Appends '258EAFA5-E914-47DA-95CA-C5AB0DC85B11' to Sec-WebSocket-Key
      from the client's request header
  2. Applies the key to the SHA-1 hashing function
  3. Encodes results with Base64
  """
  WS_UUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
  key = ws_key + WS_UUID
  hash_sha1 = hashlib.sha1(key.encode('utf-8')).digest()
  return base64.b64encode(hash_sha1).decode('ascii')


def websocket_handshake_response(ws_key):
  """ Create websocket handshake response """
  ws_hash = websocket_hash(ws_key)
  headers = {
      "Upgrade": "websocket",
      "Connection": "Upgrade",
      "Sec-WebSocket-Accept": ws_hash
  }
  return http_form_response(101, headers)


def websocket_frame_fin_bit(data_frame):
  """ WebSocket Frame Fin Bit """
  return data_frame[0] >> 7


def websocket_frame_rsv_bit(data_frame):
  """ WebSocket Frame Reserve Bit """
  return (data_frame[0] ^ 0x80) >> 4


def websocket_frame_op_code(data_frame):
  """ WebSocket Frame OP code """
  return data_frame[0] & 0x0F


def websocket_frame_mask_enabled(data_frame):
  """ WebSocket Frame Mask Enabled """
  return data_frame[1] >> 7


def websocket_apply_mask(data: bytes, mask: bytes) -> bytes:
  """
  Apply masking to the data of a WebSocket message.
  Args:
      data: data to mask.
      mask: 4-bytes mask.
  """
  if len(mask) != 4:
    raise ValueError("mask must contain 4 bytes")

  data_int = int.from_bytes(data, sys.byteorder)
  mask_repeated = mask * (len(data) // 4) + mask[:len(data) % 4]
  mask_int = int.from_bytes(mask_repeated, sys.byteorder)
  return (data_int ^ mask_int).to_bytes(len(data), sys.byteorder)


def websocket_encode_frame(payload, **kwargs):
  """
  WebSocket Frame Format:

     0                   1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-------+-+-------------+-------------------------------+
    |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
    |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
    |N|V|V|V|       |S|             |   (if payload len==126/127)   |
    | |1|2|3|       |K|             |                               |
    +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
    |     Extended payload length continued, if payload len == 127  |
    + - - - - - - - - - - - - - - - +-------------------------------+
    |                               |Masking-key, if MASK set to 1  |
    +-------------------------------+-------------------------------+
    | Masking-key (continued)       |          Payload Data         |
    +-------------------------------- - - - - - - - - - - - - - - - +
    :                     Payload Data continued ...                :
    + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
    |                     Payload Data continued ...                |
    +---------------------------------------------------------------+

  The MASK bit tells whether the message is encoded. Messages from the client
  must be masked, so your server must expect this to be 1. (In fact, section
  5.1 of the spec says that your server must disconnect from a client if that
  client sends an unmasked message.) When sending a frame back to the client,
  do not mask it and do not set the mask bit. We'll explain masking later.
  Note: You must mask messages even when using a secure socket. RSV1-3 can be
  ignored, they are for extensions.

  The opcode field defines how to interpret the payload data: 0x0 for
  continuation, 0x1 for text (which is always encoded in UTF-8), 0x2 for
  binary, and other so-called "control codes" that will be discussed later. In
  this version of WebSockets, 0x3 to 0x7 and 0xB to 0xF have no meaning.

  The FIN bit tells whether this is the last message in a series. If it's 0,
  then the server keeps listening for more parts of the message; otherwise, the
  server should consider the message delivered. More on this later.

  Source:

    https://datatracker.ietf.org/doc/html/rfc6455#section-5.1
    https://websockets.readthedocs.io/en/7.0/_modules/websockets/framing.html

  """
  fin = kwargs.get("fin", 1)  # Assume last frame
  rsv1 = kwargs.get("rsv1", 0)
  rsv2 = kwargs.get("rsv2", 0)
  rsv3 = kwargs.get("rsv3", 0)
  opcode = kwargs.get("opcode", 0x1)  # Assume text data
  mask = kwargs.get("mask", 0)

  # Form WebSocket Frame
  frame = io.BytesIO()
  length = len(payload)

  # -- Header
  # yapf:disable
  head1 = ((0b10000000 if fin else 0)
           | (0b01000000 if rsv1 else 0)
           | (0b00100000 if rsv2 else 0)
           | (0b00010000 if rsv3 else 0)
           | opcode)
  head2 = 0b10000000 if mask else 0
  # yapf:enable
  if length < 126:
    frame.write(struct.pack('!BB', head1, head2 | length))
  elif length < 65536:
    frame.write(struct.pack('!BBH', head1, head2 | 126, length))
  else:
    frame.write(struct.pack('!BBQ', head1, head2 | 127, length))

  # -- Payload
  if mask:
    mask_bits = struct.pack('!I', random.getrandbits(32))
    masked_payload = websocket_apply_mask(payload, mask_bits)
    frame.write(mask_bits)
    frame.write(masked_payload)
  else:
    frame.write(str.encode(payload))

  return frame.getvalue()


def websocket_decode_frame(reader, mask):
  """
  Decode WebSocket Frame

  To read the payload data, you must know when to stop reading. That's why the
  payload length is important to know. Unfortunately, this is somewhat
  complicated. To read it, follow these steps:

  1. Read bits 9-15 (inclusive) and interpret that as an unsigned integer. If
  it's 125 or less, then that's the length; you're done. If it's 126, go to
  step 2. If it's 127, go to step 3.

  2. Read the next 16 bits and interpret those as an unsigned integer. You're
  done.

  3. Read the next 64 bits and interpret those as an unsigned integer. (The
  most significant bit must be 0.) You're done.
  """
  # Read the header.
  data = yield from reader(2)
  head1, head2 = struct.unpack('!BB', data)

  # -- While not Pythonic, this is marginally faster than calling bool().
  fin = True if head1 & 0b10000000 else False
  rsv1 = True if head1 & 0b01000000 else False
  rsv2 = True if head1 & 0b00100000 else False
  rsv3 = True if head1 & 0b00010000 else False
  opcode = head1 & 0b00001111

  if (True if head2 & 0b10000000 else False) != mask:
    raise RuntimeError("Incorrect masking")

  length = head2 & 0b01111111
  if length == 126:
    data = yield from reader(2)
    length, = struct.unpack('!H', data)
  elif length == 127:
    data = yield from reader(8)
    length, = struct.unpack('!Q', data)

  if mask:
    mask_bits = yield from reader(4)

  # Read payload
  data = yield from reader(length)
  if mask:
    data = websocket_apply_mask(data, mask_bits)

  return data


class DebugServer:
  """ Debug Server """
  def __init__(self, callback, **kwargs):
    self.host = kwargs.get("host", '127.0.0.1')
    self.port = kwargs.get("port", 5000)
    self.callback = callback

    # Setup TCP server and start listening
    self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    self.sock.bind((self.host, self.port))
    self.sock.listen()
    self.conn, self.client_addr = self.sock.accept()

    # Get WebSocket handshake request
    buf_size = 4096
    request_string = self.conn.recv(buf_size, 0).decode("ascii")
    (_, _, _, headers) = http_parse_request(request_string)
    if "Sec-WebSocket-Key" not in headers:
      raise RuntimeError("Debug server is expecting a websocket handshake!")

    # Respond to handshake request and establish connection
    ws_key = headers["Sec-WebSocket-Key"]
    resp = websocket_handshake_response(ws_key)
    self.conn.send(str.encode(resp))

    # Loop
    while True:
      payload = callback()
      frame = websocket_encode_frame(payload)
      self.conn.send(frame)

  def __enter__(self):
    return self

  def __exit__(self, exc_type, exc_value, traceback):
    self.conn.close()
    self.sock.close()


###############################################################################
# MATHS
###############################################################################

from math import pi
from math import isclose
from math import sqrt
# from math import floor
from math import cos
from math import sin
from math import tan
from math import acos
from math import atan
from math import atan2


def rmse(errors):
  """ Root Mean Squared Error """
  return np.sqrt(np.mean(errors**2))


###############################################################################
# LINEAR ALGEBRA
###############################################################################

from numpy import rad2deg
from numpy import deg2rad
from numpy import sinc
from numpy import zeros
from numpy import ones
from numpy import eye
from numpy import trace
from numpy import diagonal as diag
from numpy import cross
from numpy.linalg import norm
from numpy.linalg import inv
from numpy.linalg import pinv
from numpy.linalg import matrix_rank as rank
from numpy.linalg import eig
from numpy.linalg import svd
from numpy.linalg import cholesky as chol


def normalize(v):
  """ Normalize vector v """
  n = np.linalg.norm(v)
  if n == 0:
    return v
  return v / n


def full_rank(A):
  """ Check if matrix A is full rank """
  return rank(A) == A.shape[0]


def skew(vec):
  """ Form skew-symmetric matrix from vector `vec` """
  assert vec.shape == (3,) or vec.shape == (3, 1)
  x, y, z = vec
  return np.array([[0.0, -z, y], [z, 0.0, -x], [-y, x, 0.0]])


def skew_inv(A):
  """ Form skew symmetric matrix vector """
  assert A.shape == (3, 3)
  return np.array([A[2, 1], A[0, 2], A[1, 0]])


def fwdsubs(L, b):
  """
  Solving a lower triangular system by forward-substitution
  Input matrix L is an n by n lower triangular matrix
  Input vector b is n by 1
  Output vector x is the solution to the linear system
  L x = b
  """
  assert L.shape[1] == b.shape[0]
  n = b.shape[0]

  x = zeros((n, 1))
  for j in range(n):
    if L[j, j] == 0:
      raise RuntimeError('Matrix is singular!')
    x[j] = b[j] / L[j, j]
    b[j:n] = b[j:n] - L[j:n, j] * x[j]


def bwdsubs(U, b):
  """
  Solving an upper triangular system by back-substitution
  Input matrix U is an n by n upper triangular matrix
  Input vector b is n by 1
  Output vector x is the solution to the linear system
  U x = b
  """
  assert U.shape[1] == b.shape[0]
  n = b.shape[0]

  x = zeros((n, 1))
  for j in range(n):
    if U[j, j] == 0:
      raise RuntimeError('Matrix is singular!')
    x[j] = b[j] / U(j, j)
    b[0:j] = b[0:j] - U[0:j, j] * x[j]


def solve_svd(A, b):
  """
  Solve Ax = b with SVD
  """
  # compute svd of A
  U, s, Vh = svd(A)

  # U diag(s) Vh x = b <=> diag(s) Vh x = U.T b = c
  c = np.dot(U.T, b)

  # diag(s) Vh x = c <=> Vh x = diag(1/s) c = w (trivial inversion of a diagonal matrix)
  w = np.dot(np.diag(1 / s), c)

  # Vh x = w <=> x = Vh.H w (where .H stands for hermitian = conjugate transpose)
  x = np.dot(Vh.conj().T, w)

  return x


def schurs_complement(H, g, m, r, precond=False):
  """ Shurs-complement """
  assert H.shape[0] == (m + r)

  # H = [Hmm, Hmr
  #      Hrm, Hrr];
  Hmm = H[0:m, 0:m]
  Hmr = H[0:m, m:]
  Hrm = Hmr.T
  Hrr = H[m:, m:]

  # g = [gmm, grr]
  gmm = g[1:]
  grr = g[m:]

  # Precondition Hmm
  if precond:
    Hmm = 0.5 * (Hmm + Hmm.T)

  # Invert Hmm
  assert rank(Hmm) == Hmm.shape[0]
  (w, V) = eig(Hmm)
  W_inv = diag(1.0 / w)
  Hmm_inv = V * W_inv * V.T

  # Schurs complement
  H_marg = Hrr - Hrm * Hmm_inv * Hmr
  g_marg = grr - Hrm * Hmm_inv * gmm

  return (H_marg, g_marg)


def is_pd(B):
  """Returns true when input is positive-definite, via Cholesky"""
  try:
    _ = chol(B)
    return True
  except np.linalg.LinAlgError:
    return False


def nearest_pd(A):
  """Find the nearest positive-definite matrix to input

  A Python/Numpy port of John D'Errico's `nearestSPD` MATLAB code [1], which
  credits [2].

  [1] https://www.mathworks.com/matlabcentral/fileexchange/42885-nearestspd

  [2] N.J. Higham, "Computing a nearest symmetric positive semidefinite
  matrix" (1988): https://doi.org/10.1016/0024-3795(88)90223-6
  """
  B = (A + A.T) / 2
  _, s, V = svd(B)
  H = np.dot(V.T, np.dot(np.diag(s), V))
  A2 = (B + H) / 2
  A3 = (A2 + A2.T) / 2

  if is_pd(A3):
    return A3

  spacing = np.spacing(np.linalg.norm(A))
  # The above is different from [1]. It appears that MATLAB's `chol` Cholesky
  # decomposition will accept matrixes with exactly 0-eigenvalue, whereas
  # Numpy's will not. So where [1] uses `eps(mineig)` (where `eps` is Matlab
  # for `np.spacing`), we use the above definition. CAVEAT: our `spacing`
  # will be much larger than [1]'s `eps(mineig)`, since `mineig` is usually on
  # the order of 1e-16, and `eps(1e-16)` is on the order of 1e-34, whereas
  # `spacing` will, for Gaussian random matrixes of small dimension, be on
  # othe order of 1e-16. In practice, both ways converge, as the unit test
  # below suggests.
  I = np.eye(A.shape[0])
  k = 1
  while not is_pd(A3):
    mineig = np.min(np.real(np.linalg.eigvals(A3)))
    A3 += I * (-mineig * k**2 + spacing)
    k += 1

  return A3


def matrix_equal(A, B, tol=1e-8, verbose=False):
  """ Compare matrices `A` and `B` """
  diff = A - B

  if len(diff.shape) == 1:
    for i in range(diff.shape[0]):
      if abs(diff[i]) > tol:
        if verbose:
          print("A - B:")
          print(diff)

  elif len(diff.shape) == 2:
    for i in range(diff.shape[0]):
      for j in range(diff.shape[1]):
        if abs(diff[i, j]) > tol:
          if verbose:
            print("A - B:")
            print(diff)
          return False

  return True


def plot_compare_matrices(title_A, A, title_B, B):
  """ Plot compare matrices """
  plt.matshow(A)
  plt.colorbar()
  plt.title(title_A)

  plt.matshow(B)
  plt.colorbar()
  plt.title(title_B)

  diff = A - B
  plt.matshow(diff)
  plt.colorbar()
  plt.title(f"{title_A} - {title_B}")

  print(f"max_coeff({title_A}): {np.max(np.max(A))}")
  print(f"max_coeff({title_B}): {np.max(np.max(B))}")
  print(f"min_coeff({title_A}): {np.min(np.min(A))}")
  print(f"min_coeff({title_B}): {np.min(np.min(B))}")
  print(f"max_diff: {np.max(np.max(np.abs(diff)))}")

  plt.show()


def check_jacobian(jac_name, fdiff, jac, threshold, verbose=False):
  """ Check jacobians """

  # Check if numerical diff is same as analytical jacobian
  if matrix_equal(fdiff, jac, threshold):
    if verbose:
      print(f"Check [{jac_name}] passed!")
    return True

  # Failed - print differences
  if verbose:
    fdiff_minus_jac = fdiff - jac

    print(f"Check [{jac_name}] failed!")
    print("-" * 60)

    print("J_fdiff - J:")
    print(np.round(fdiff_minus_jac, 4))
    print()

    print("J_fdiff:")
    print(np.round(fdiff, 4))
    print()

    print("J:")
    print(np.round(jac, 4))
    print()

    print("-" * 60)

  return False


###############################################################################
# GEOMETRY
###############################################################################


def lerp(x0, x1, t):
  """ Linear interpolation """
  return (1.0 - t) * x0 + t * x1


def lerp2d(p0, p1, t):
  """ Linear interpolation 2D """
  assert len(p0) == 2
  assert len(p1) == 2
  assert t <= 1.0 and t >= 0.0
  x = lerp(p0[0], p1[0], t)
  y = lerp(p0[1], p1[1], t)
  return np.array([x, y])


def lerp3d(p0, p1, t):
  """ Linear interpolation 3D """
  assert len(p0) == 3
  assert len(p1) == 3
  assert t <= 1.0 and t >= 0.0
  x = lerp(p0[0], p1[0], t)
  y = lerp(p0[1], p1[1], t)
  z = lerp(p0[2], p1[2], t)
  return np.array([x, y, z])


def circle(r, theta):
  """ Circle """
  x = r * cos(theta)
  y = r * sin(theta)
  return np.array([x, y])


def sphere(rho, theta, phi):
  """
  Sphere

  Args:

    rho (float): Sphere radius
    theta (float): longitude [rad]
    phi (float): Latitude [rad]

  Returns:

    Point on sphere

  """
  x = rho * sin(theta) * cos(phi)
  y = rho * sin(theta) * sin(phi)
  z = rho * cos(theta)
  return np.array([x, y, z])


def circle_loss(c, x, y):
  """
    Calculate the algebraic distance between the data points and the mean
    circle centered at c=(xc, yc)
    """
  xc, yc = c
  # Euclidean dist from center (xc, yc)
  Ri = np.sqrt((x - xc)**2 + (y - yc)**2)
  return Ri - Ri.mean()


def find_circle(x, y):
  """
    Find the circle center and radius given (x, y) data points using least
    squares. Returns `(circle_center, circle_radius, residual)`
    """
  x_m = np.mean(x)
  y_m = np.mean(y)
  center_init = x_m, y_m
  center, _ = scipy.optimize.leastsq(circle_loss, center_init, args=(x, y))

  xc, yc = center
  radii = np.sqrt((x - xc)**2 + (y - yc)**2)
  radius = radii.mean()
  residual = np.sum((radii - radius)**2)

  return (center, radius, residual)


def bresenham(p0, p1):
  """
    Bresenham's line algorithm is a line drawing algorithm that determines the
    points of an n-dimensional raster that should be selected in order to form
    a close approximation to a straight line between two points. It is commonly
    used to draw line primitives in a bitmap image (e.g. on a computer screen),
    as it uses only integer addition, subtraction and bit shifting, all of
    which are very cheap operations in standard computer architectures.

    Args:

      p0 (np.array): Starting point (x, y)
      p1 (np.array): End point (x, y)

    Returns:

      A list of (x, y) intermediate points from p0 to p1.

  """
  x0, y0 = p0
  x1, y1 = p1
  dx = abs(x1 - x0)
  dy = abs(y1 - y0)
  sx = 1.0 if x0 < x1 else -1.0
  sy = 1.0 if y0 < y1 else -1.0
  err = dx - dy

  line = []
  while True:
    line.append([x0, y0])
    if x0 == x1 and y0 == y1:
      return line

    e2 = 2 * err
    if e2 > -dy:
      # overshot in the y direction
      err = err - dy
      x0 = x0 + sx
    if e2 < dx:
      # overshot in the x direction
      err = err + dx
      y0 = y0 + sy


###############################################################################
# LIE
###############################################################################


def Exp(phi):
  """ Exponential Map """
  assert phi.shape == (3,) or phi.shape == (3, 1)
  if norm(phi) < 1e-3:
    C = eye(3) + skew(phi)
    return C

  phi_norm = norm(phi)
  phi_skew = skew(phi)
  phi_skew_sq = phi_skew @ phi_skew

  C = eye(3)
  C += (sin(phi_norm) / phi_norm) * phi_skew
  C += ((1 - cos(phi_norm)) / phi_norm**2) * phi_skew_sq
  return C


def Log(C):
  """ Logarithmic Map """
  assert C.shape == (3, 3)
  # phi = acos((trace(C) - 1) / 2);
  # u = skew_inv(C - C') / (2 * sin(phi));
  # rvec = phi * u;

  C00, C01, C02 = C[0, :]
  C10, C11, C12 = C[1, :]
  C20, C21, C22 = C[2, :]

  tr = np.trace(C)
  rvec = None
  if tr + 1.0 < 1e-10:
    if abs(C22 + 1.0) > 1.0e-5:
      x = np.array([C02, C12, 1.0 + C22])
      rvec = (pi / np.sqrt(2.0 + 2.0 * C22)) @ x
    elif abs(C11 + 1.0) > 1.0e-5:
      x = np.array([C01, 1.0 + C11, C21])
      rvec = (pi / np.sqrt(2.0 + 2.0 * C11)) @ x
    else:
      x = np.array([1.0 + C00, C10, C20])
      rvec = (pi / np.sqrt(2.0 + 2.0 * C00)) @ x

  else:
    tr_3 = tr - 3.0  # always negative
    if tr_3 < -1e-7:
      theta = acos((tr - 1.0) / 2.0)
      magnitude = theta / (2.0 * sin(theta))
    else:
      # when theta near 0, +-2pi, +-4pi, etc. (trace near 3.0)
      # use Taylor expansion: theta \approx 1/2-(t-3)/12 + O((t-3)^2)
      # see https://github.com/borglab/gtsam/issues/746 for details
      magnitude = 0.5 - tr_3 / 12.0
    rvec = magnitude @ np.array([C21 - C12, C02 - C20, C10 - C01])

  return rvec


def Jr(theta):
  """
  Right jacobian

  Forster, Christian, et al. "IMU preintegration on manifold for efficient
  visual-inertial maximum-a-posteriori estimation." Georgia Institute of
  Technology, 2015.
  [Page 2, Equation (8)]
  """
  theta_norm = norm(theta)
  theta_norm_sq = theta_norm * theta_norm
  theta_norm_cube = theta_norm_sq * theta_norm
  theta_skew = skew(theta)
  theta_skew_sq = theta_skew @ theta_skew

  J = eye(3)
  J -= ((1 - cos(theta_norm)) / theta_norm_sq) * theta_skew
  J += (theta_norm - sin(theta_norm)) / (theta_norm_cube) * theta_skew_sq
  return J


def Jr_inv(theta):
  """ Inverse right jacobian """
  theta_norm = norm(theta)
  theta_norm_sq = theta_norm * theta_norm
  theta_skew = skew(theta)
  theta_skew_sq = theta_skew @ theta_skew

  A = 1.0 / theta_norm_sq
  B = (1 + cos(theta_norm)) / (2 * theta_norm * sin(theta_norm))

  J = eye(3)
  J += 0.5 * theta_skew
  J += (A - B) * theta_skew_sq
  return J


def boxplus(C, alpha):
  """ Box plus """
  # C_updated = C [+] alpha
  C_updated = C * Exp(alpha)
  return C_updated


def boxminus(C_a, C_b):
  """ Box minus """
  # alpha = C_a [-] C_b
  alpha = Log(inv(C_b) * C_a)
  return alpha


###############################################################################
# TRANSFORM
###############################################################################


def homogeneous(p):
  """ Turn point `p` into its homogeneous form """
  return np.array([*p, 1.0])


def dehomogeneous(hp):
  """ De-homogenize point `hp` into `p` """
  return hp[0:3]


def rotx(theta):
  """ Form rotation matrix around x axis """
  row0 = [1.0, 0.0, 0.0]
  row1 = [0.0, cos(theta), -sin(theta)]
  row2 = [0.0, sin(theta), cos(theta)]
  return np.array([row0, row1, row2])


def roty(theta):
  """ Form rotation matrix around y axis """
  row0 = [cos(theta), 0.0, sin(theta)]
  row1 = [0.0, 1.0, 0.0]
  row2 = [-sin(theta), 0.0, cos(theta)]
  return np.array([row0, row1, row2])


def rotz(theta):
  """ Form rotation matrix around z axis """
  row0 = [cos(theta), -sin(theta), 0.0]
  row1 = [sin(theta), cos(theta), 0.0]
  row2 = [0.0, 0.0, 1.0]
  return np.array([row0, row1, row2])


def aa2quat(angle, axis):
  """
  Convert angle-axis to quaternion

  Source:
  Sola, Joan. "Quaternion kinematics for the error-state Kalman filter." arXiv
  preprint arXiv:1711.02508 (2017).
  [Page 22, eq (101), "Quaternion and rotation vector"]
  """
  ax, ay, az = axis
  qw = cos(angle / 2.0)
  qx = ax * sin(angle / 2.0)
  qy = ay * sin(angle / 2.0)
  qz = az * sin(angle / 2.0)
  return np.array([qw, qx, qy, qz])


def rvec2rot(rvec):
  """ Rotation vector to rotation matrix """
  # If small rotation
  theta = sqrt(rvec @ rvec)  # = norm(rvec), but faster
  eps = 1e-8
  if theta < eps:
    return skew(rvec)

  # Convert rvec to rotation matrix
  rvec = rvec / theta
  x, y, z = rvec

  c = cos(theta)
  s = sin(theta)
  C = 1 - c

  xs = x * s
  ys = y * s
  zs = z * s

  xC = x * C
  yC = y * C
  zC = z * C

  xyC = x * yC
  yzC = y * zC
  zxC = z * xC

  row0 = [x * xC + c, xyC - zs, zxC + ys]
  row1 = [xyC + zs, y * yC + c, yzC - xs]
  row2 = [zxC - ys, yzC + xs, z * zC + c]
  return np.array([row0, row1, row2])


def vecs2axisangle(u, v):
  """ From 2 vectors form an axis-angle vector """
  angle = math.acos(u.T * v)
  ax = normalize(np.cross(u, v))
  return ax * angle


def euler321(yaw, pitch, roll):
  """
  Convert yaw, pitch, roll in radians to a 3x3 rotation matrix.

  Source:
  Kuipers, Jack B. Quaternions and Rotation Sequences: A Primer with
  Applications to Orbits, Aerospace, and Virtual Reality. Princeton, N.J:
  Princeton University Press, 1999. Print.
  [Page 85-86, "The Aerospace Sequence"]
  """
  psi = yaw
  theta = pitch
  phi = roll

  cpsi = cos(psi)
  spsi = sin(psi)
  ctheta = cos(theta)
  stheta = sin(theta)
  cphi = cos(phi)
  sphi = sin(phi)

  C11 = cpsi * ctheta
  C21 = spsi * ctheta
  C31 = -stheta

  C12 = cpsi * stheta * sphi - spsi * cphi
  C22 = spsi * stheta * sphi + cpsi * cphi
  C32 = ctheta * sphi

  C13 = cpsi * stheta * cphi + spsi * sphi
  C23 = spsi * stheta * cphi - cpsi * sphi
  C33 = ctheta * cphi

  return np.array([[C11, C12, C13], [C21, C22, C23], [C31, C32, C33]])


def euler2quat(yaw, pitch, roll):
  """
  Convert yaw, pitch, roll in radians to a quaternion.

  Source:
  Kuipers, Jack B. Quaternions and Rotation Sequences: A Primer with
  Applications to Orbits, Aerospace, and Virtual Reality. Princeton, N.J:
  Princeton University Press, 1999. Print.
  [Page 166-167, "Euler Angles to Quaternion"]
  """
  psi = yaw  # Yaw
  theta = pitch  # Pitch
  phi = roll  # Roll

  c_phi = cos(phi / 2.0)
  c_theta = cos(theta / 2.0)
  c_psi = cos(psi / 2.0)
  s_phi = sin(phi / 2.0)
  s_theta = sin(theta / 2.0)
  s_psi = sin(psi / 2.0)

  qw = c_psi * c_theta * c_phi + s_psi * s_theta * s_phi
  qx = c_psi * c_theta * s_phi - s_psi * s_theta * c_phi
  qy = c_psi * s_theta * c_phi + s_psi * c_theta * s_phi
  qz = s_psi * c_theta * c_phi - c_psi * s_theta * s_phi

  mag = sqrt(qw**2 + qx**2 + qy**2 + qz**2)
  return np.array([qw / mag, qx / mag, qy / mag, qz / mag])


def quat2euler(q):
  """
  Convert quaternion to euler angles (yaw, pitch, roll).

  Source:
  Kuipers, Jack B. Quaternions and Rotation Sequences: A Primer with
  Applications to Orbits, Aerospace, and Virtual Reality. Princeton, N.J:
  Princeton University Press, 1999. Print.
  [Page 168, "Quaternion to Euler Angles"]
  """
  qw, qx, qy, qz = q

  m11 = (2 * qw**2) + (2 * qx**2) - 1
  m12 = 2 * (qx * qy + qw * qz)
  m13 = 2 * qx * qz - 2 * qw * qy
  m23 = 2 * qy * qz + 2 * qw * qx
  m33 = (2 * qw**2) + (2 * qz**2) - 1

  psi = math.atan2(m12, m11)
  theta = math.asin(-m13)
  phi = math.atan2(m23, m33)

  ypr = np.array([psi, theta, phi])
  return ypr


def quat2rot(q):
  """
  Convert quaternion to 3x3 rotation matrix.

  Source:
  Blanco, Jose-Luis. "A tutorial on se (3) transformation parameterizations
  and on-manifold optimization." University of Malaga, Tech. Rep 3 (2010): 6.
  [Page 18, Equation (2.20)]
  """
  assert len(q) == 4
  qw, qx, qy, qz = q

  qx2 = qx**2
  qy2 = qy**2
  qz2 = qz**2
  qw2 = qw**2

  # Homogeneous form
  C11 = qw2 + qx2 - qy2 - qz2
  C12 = 2.0 * (qx * qy - qw * qz)
  C13 = 2.0 * (qx * qz + qw * qy)

  C21 = 2.0 * (qx * qy + qw * qz)
  C22 = qw2 - qx2 + qy2 - qz2
  C23 = 2.0 * (qy * qz - qw * qx)

  C31 = 2.0 * (qx * qz - qw * qy)
  C32 = 2.0 * (qy * qz + qw * qx)
  C33 = qw2 - qx2 - qy2 + qz2

  return np.array([[C11, C12, C13], [C21, C22, C23], [C31, C32, C33]])


def rot2euler(C):
  """
  Convert 3x3 rotation matrix to euler angles (yaw, pitch, roll).
  """
  assert C.shape == (3, 3)
  q = rot2quat(C)
  return quat2euler(q)


def rot2quat(C):
  """
  Convert 3x3 rotation matrix to quaternion.
  """
  assert C.shape == (3, 3)

  m00 = C[0, 0]
  m01 = C[0, 1]
  m02 = C[0, 2]

  m10 = C[1, 0]
  m11 = C[1, 1]
  m12 = C[1, 2]

  m20 = C[2, 0]
  m21 = C[2, 1]
  m22 = C[2, 2]

  tr = m00 + m11 + m22

  if tr > 0:
    S = sqrt(tr + 1.0) * 2.0
    # S=4*qw
    qw = 0.25 * S
    qx = (m21 - m12) / S
    qy = (m02 - m20) / S
    qz = (m10 - m01) / S
  elif ((m00 > m11) and (m00 > m22)):
    S = sqrt(1.0 + m00 - m11 - m22) * 2.0
    # S=4*qx
    qw = (m21 - m12) / S
    qx = 0.25 * S
    qy = (m01 + m10) / S
    qz = (m02 + m20) / S
  elif m11 > m22:
    S = sqrt(1.0 + m11 - m00 - m22) * 2.0
    # S=4*qy
    qw = (m02 - m20) / S
    qx = (m01 + m10) / S
    qy = 0.25 * S
    qz = (m12 + m21) / S
  else:
    S = sqrt(1.0 + m22 - m00 - m11) * 2.0
    # S=4*qz
    qw = (m10 - m01) / S
    qx = (m02 + m20) / S
    qy = (m12 + m21) / S
    qz = 0.25 * S

  return quat_normalize(np.array([qw, qx, qy, qz]))


# QUATERNION ##################################################################


def quat_norm(q):
  """ Returns norm of a quaternion """
  qw, qx, qy, qz = q
  return sqrt(qw**2 + qx**2 + qy**2 + qz**2)


def quat_normalize(q):
  """ Normalize quaternion """
  n = quat_norm(q)
  qw, qx, qy, qz = q
  return np.array([qw / n, qx / n, qy / n, qz / n])


def quat_conj(q):
  """ Return conjugate quaternion """
  qw, qx, qy, qz = q
  q_conj = np.array([qw, -qx, -qy, -qz])
  return q_conj


def quat_inv(q):
  """ Invert quaternion """
  return quat_conj(q)


def quat_left(q):
  """ Quaternion left product matrix """
  qw, qx, qy, qz = q
  row0 = [qw, -qx, -qy, -qz]
  row1 = [qx, qw, -qz, qy]
  row2 = [qy, qz, qw, -qx]
  row3 = [qz, -qy, qx, qw]
  return np.array([row0, row1, row2, row3])


def quat_right(q):
  """ Quaternion right product matrix """
  qw, qx, qy, qz = q
  row0 = [qw, -qx, -qy, -qz]
  row1 = [qx, qw, qz, -qy]
  row2 = [qy, -qz, qw, qx]
  row3 = [qz, qy, -qx, qw]
  return np.array([row0, row1, row2, row3])


def quat_lmul(p, q):
  """ Quaternion left multiply """
  assert len(p) == 4
  assert len(q) == 4
  lprod = quat_left(p)
  return lprod @ q


def quat_rmul(p, q):
  """ Quaternion right multiply """
  assert len(p) == 4
  assert len(q) == 4
  rprod = quat_right(q)
  return rprod @ p


def quat_mul(p, q):
  """ Quaternion multiply p * q """
  return quat_lmul(p, q)


def quat_omega(w):
  """ Quaternion omega matrix """
  return np.block([[-1.0 * skew(w), w], [w.T, 0.0]])


def quat_delta(dalpha):
  """ Form quaternion from small angle rotation vector dalpha """
  half_norm = 0.5 * norm(dalpha)
  scalar = cos(half_norm)
  vector = sinc(half_norm) * 0.5 * dalpha

  dqw = scalar
  dqx, dqy, dqz = vector
  dq = np.array([dqw, dqx, dqy, dqz])

  return dq


def quat_integrate(q_k, w, dt):
  """
  Sola, Joan. "Quaternion kinematics for the error-state Kalman filter." arXiv
  preprint arXiv:1711.02508 (2017).
  [Section 4.6.1 Zeroth-order integration, p.47]
  """
  w_norm = norm(w)
  q_scalar = 0.0
  q_vec = np.array([0.0, 0.0, 0.0])

  if w_norm > 1e-5:
    q_scalar = cos(w_norm * dt * 0.5)
    q_vec = w / w_norm * sin(w_norm * dt * 0.5)
  else:
    q_scalar = 1.0
    q_vec = [0.0, 0.0, 0.0]

  q_kp1 = quat_mul(q_k, np.array([q_scalar, q_vec]))
  return q_kp1


def quat_slerp(q_i, q_j, t):
  """ Quaternion Slerp `q_i` and `q_j` with parameter `t` """
  assert len(q_i) == 4
  assert len(q_j) == 4
  assert t >= 0.0 and t <= 1.0

  # Compute the cosine of the angle between the two vectors.
  dot_result = q_i @ q_j

  # If the dot product is negative, slerp won't take
  # the shorter path. Note that q_j and -q_j are equivalent when
  # the negation is applied to all four components. Fix by
  # reversing one quaternion.
  if dot_result < 0.0:
    q_j = -q_j
    dot_result = -dot_result

  DOT_THRESHOLD = 0.9995
  if dot_result > DOT_THRESHOLD:
    # If the inputs are too close for comfort, linearly interpolate
    # and normalize the result.
    return q_i + t * (q_j - q_i)

  # Since dot is in range [0, DOT_THRESHOLD], acos is safe
  theta_0 = acos(dot_result)  # theta_0 = angle between input vectors
  theta = theta_0 * t  # theta = angle between q_i and result
  sin_theta = sin(theta)  # compute this value only once
  sin_theta_0 = sin(theta_0)  # compute this value only once

  # == sin(theta_0 - theta) / sin(theta_0)
  s0 = cos(theta) - dot_result * sin_theta / sin_theta_0
  s1 = sin_theta / sin_theta_0

  return (s0 * q_i) + (s1 * q_j)


# TF ##########################################################################


def tf(rot, trans):
  """
  Form 4x4 homogeneous transformation matrix from rotation `rot` and
  translation `trans`. Where the rotation component `rot` can be a rotation
  matrix or a quaternion.
  """
  C = None
  if rot.shape == (4,) or rot.shape == (4, 1):
    C = quat2rot(rot)
  elif rot.shape == (3, 3):
    C = rot
  else:
    raise RuntimeError("Invalid rotation!")

  T = np.eye(4, 4)
  T[0:3, 0:3] = C
  T[0:3, 3] = trans
  return T


def tf_rot(T):
  """ Return rotation matrix from 4x4 homogeneous transform """
  assert T.shape == (4, 4)
  return T[0:3, 0:3]


def tf_quat(T):
  """ Return quaternion from 4x4 homogeneous transform """
  assert T.shape == (4, 4)
  return rot2quat(tf_rot(T))


def tf_trans(T):
  """ Return translation vector from 4x4 homogeneous transform """
  assert T.shape == (4, 4)
  return T[0:3, 3]


def tf_inv(T):
  """ Invert 4x4 homogeneous transform """
  assert T.shape == (4, 4)
  return np.linalg.inv(T)


def tf_point(T, p):
  """ Transform 3d point """
  assert T.shape == (4, 4)
  assert p.shape == (3,) or p.shape == (3, 1)
  hpoint = np.array([p[0], p[1], p[2], 1.0])
  return (T @ hpoint)[0:3]


def tf_hpoint(T, hp):
  """ Transform 3d point """
  assert T.shape == (4, 4)
  assert hp.shape == (4,) or hp.shape == (4, 1)
  return (T @ hp)[0:3]


def tf_decompose(T):
  """ Decompose into rotation matrix and translation vector"""
  assert T.shape == (4, 4)
  C = tf_rot(T)
  r = tf_trans(T)
  return (C, r)


def tf_lerp(pose_i, pose_j, t):
  """ Interpolate pose `pose_i` and `pose_j` with parameter `t` """
  assert pose_i.shape == (4, 4)
  assert pose_j.shape == (4, 4)
  assert t >= 0.0 and t <= 1.0

  # Decompose start pose
  r_i = tf_trans(pose_i)
  q_i = tf_quat(pose_i)

  # Decompose end pose
  r_j = tf_trans(pose_j)
  q_j = tf_quat(pose_j)

  # Interpolate translation and rotation
  r_lerp = lerp(r_i, r_j, t)
  q_lerp = quat_slerp(q_i, q_j, t)

  return tf(q_lerp, r_lerp)


def tf_perturb(T, i, step_size):
  """ Perturb transformation matrix """
  assert T.shape == (4, 4)
  assert i >= 0 and i <= 5

  # Setup
  C = tf_rot(T)
  r = tf_trans(T)

  if i >= 0 and i <= 2:
    # Perturb translation
    r[i] += step_size

  elif i >= 3 and i <= 5:
    # Perturb rotation
    rvec = np.array([0.0, 0.0, 0.0])
    rvec[i - 3] = step_size

    q = rot2quat(C)
    dq = quat_delta(rvec)

    q_diff = quat_mul(q, dq)
    q_diff = quat_normalize(q_diff)

    C = quat2rot(q_diff)

  return tf(C, r)


def tf_update(T, dx):
  """ Update transformation matrix """
  assert T.shape == (4, 4)

  q = tf_quat(T)
  r = tf_trans(T)

  dr = dx[0:3]
  dalpha = dx[3:6]
  dq = quat_delta(dalpha)

  return tf(quat_mul(q, dq), r + dr)


def load_extrinsics(csv_path):
  """ Load Extrinsics """
  csv_data = pandas.read_csv(csv_path)

  rx = csv_data["rx"]
  ry = csv_data["ry"]
  rz = csv_data["rz"]
  r = np.array([rx, ry, rz])

  qw = csv_data["qw"]
  qx = csv_data["qx"]
  qy = csv_data["qy"]
  qz = csv_data["qz"]
  q = np.array([qw, qx, qy, qz])

  return tf(q, r)


def load_poses(csv_path):
  """ Load poses """
  csv_data = pandas.read_csv(csv_path)
  pose_data = []

  for row_idx in range(csv_data.shape[0]):
    pose_ts = csv_data["#ts"][row_idx]

    rx = csv_data["rx"][row_idx]
    ry = csv_data["ry"][row_idx]
    rz = csv_data["rz"][row_idx]
    r = np.array([rx, ry, rz])

    qw = csv_data["qw"][row_idx]
    qx = csv_data["qx"][row_idx]
    qy = csv_data["qy"][row_idx]
    qz = csv_data["qz"][row_idx]
    q = np.array([qw, qx, qy, qz])

    pose_data.append((pose_ts, tf(q, r)))

  return pose_data


###############################################################################
# MATPLOTLIB
###############################################################################

import matplotlib.pylab as plt
import matplotlib.patches
import matplotlib.transforms


def plot_set_axes_equal(ax):
  """
  Make axes of 3D plot have equal scale so that spheres appear as spheres,
  cubes as cubes, etc..  This is one possible solution to Matplotlib's
  ax.set_aspect('equal') and ax.axis('equal') not working for 3D.

  Input
    ax: a matplotlib axis, e.g., as output from plt.gca().
  """
  x_limits = ax.get_xlim3d()
  y_limits = ax.get_ylim3d()
  z_limits = ax.get_zlim3d()

  x_range = abs(x_limits[1] - x_limits[0])
  x_middle = np.mean(x_limits)
  y_range = abs(y_limits[1] - y_limits[0])
  y_middle = np.mean(y_limits)
  z_range = abs(z_limits[1] - z_limits[0])
  z_middle = np.mean(z_limits)

  # The plot bounding box is a sphere in the sense of the infinity
  # norm, hence I call half the max range the plot radius.
  plot_radius = 0.5 * max([x_range, y_range, z_range])

  ax.set_xlim3d([x_middle - plot_radius, x_middle + plot_radius])
  ax.set_ylim3d([y_middle - plot_radius, y_middle + plot_radius])
  ax.set_zlim3d([z_middle - plot_radius, z_middle + plot_radius])


def confidence_ellipse(x, y, ax, n_std=3.0, facecolor='none', **kwargs):
  """
  Create a plot of the covariance confidence ellipse of *x* and *y*.

  Parameters
  ----------
  x, y : array-like, shape (n, )
      Input data.

  ax : matplotlib.axes.Axes
      The axes object to draw the ellipse into.

  n_std : float
      The number of standard deviations to determine the ellipse's radiuses.

  **kwargs
      Forwarded to `~matplotlib.patches.Ellipse`

  Returns
  -------
  matplotlib.patches.Ellipse
  """
  if x.size != y.size:
    raise ValueError("x and y must be the same size")

  cov = np.cov(x, y)
  pearson = cov[0, 1] / np.sqrt(cov[0, 0] * cov[1, 1])
  # Using a special case to obtain the eigenvalues of this
  # two-dimensionl dataset.
  ell_radius_x = np.sqrt(1 + pearson)
  ell_radius_y = np.sqrt(1 - pearson)
  ellipse = matplotlib.patches.Ellipse((0, 0),
                                       width=ell_radius_x * 2,
                                       height=ell_radius_y * 2,
                                       facecolor=facecolor,
                                       **kwargs)

  # Calculating the stdandard deviation of x from
  # the squareroot of the variance and multiplying
  # with the given number of standard deviations.
  scale_x = np.sqrt(cov[0, 0]) * n_std
  mean_x = np.mean(x)

  # calculating the stdandard deviation of y ...
  scale_y = np.sqrt(cov[1, 1]) * n_std
  mean_y = np.mean(y)

  transf = matplotlib.transforms.Affine2D() \
      .rotate_deg(45) \
      .scale(scale_x, scale_y) \
      .translate(mean_x, mean_y)

  ellipse.set_transform(transf + ax.transData)
  return ax.add_patch(ellipse)


def plot_tf(ax, T, **kwargs):
  """
  Plot 4x4 Homogeneous Transform

  Args:

    ax (matplotlib.axes.Axes): Plot axes object
    T (np.array): 4x4 homogeneous transform (i.e. Pose in the world frame)

  Keyword args:

    size (float): Size of the coordinate-axes
    linewidth (float): Thickness of the coordinate-axes
    name (str): Frame name
    name_offset (np.array or list): Position offset for displaying the frame's name
    fontsize (float): Frame font size
    fontweight (float): Frame font weight

  """
  assert T.shape == (4, 4)

  size = kwargs.get('size', 1)
  # linewidth = kwargs.get('linewidth', 3)
  name = kwargs.get('name', None)
  name_offset = kwargs.get('name_offset', [0, 0, -0.01])
  fontsize = kwargs.get('fontsize', 10)
  fontweight = kwargs.get('fontweight', 'bold')
  colors = kwargs.get('colors', ['r-', 'g-', 'b-'])

  origin = tf_trans(T)
  lx = tf_point(T, np.array([size, 0.0, 0.0]))
  ly = tf_point(T, np.array([0.0, size, 0.0]))
  lz = tf_point(T, np.array([0.0, 0.0, size]))

  # Draw x-axis
  px = [origin[0], lx[0]]
  py = [origin[1], lx[1]]
  pz = [origin[2], lx[2]]
  ax.plot(px, py, pz, colors[0])

  # Draw y-axis
  px = [origin[0], ly[0]]
  py = [origin[1], ly[1]]
  pz = [origin[2], ly[2]]
  ax.plot(px, py, pz, colors[1])

  # Draw z-axis
  px = [origin[0], lz[0]]
  py = [origin[1], lz[1]]
  pz = [origin[2], lz[2]]
  ax.plot(px, py, pz, colors[2])

  # Draw label
  if name is not None:
    x = origin[0] + name_offset[0]
    y = origin[1] + name_offset[1]
    z = origin[2] + name_offset[2]
    ax.text(x, y, z, name, fontsize=fontsize, fontweight=fontweight)


def plot_xyz(title, data, key_time, key_x, key_y, key_z, ylabel, **kwargs):
  """
  Plot XYZ plot

  Args:

    title (str): Plot title
    data (Dict[str, pandas.DataFrame]): Plot data
    key_time (str): Dictionary key for timestamps
    key_x (str): Dictionary key x-axis
    key_y (str): Dictionary key y-axis
    key_z (str): Dictionary key z-axis
    ylabel (str): Y-axis label

  """
  axis = ['x', 'y', 'z']
  colors = ["r", "g", "b"]
  keys = [key_x, key_y, key_z]
  line_styles = kwargs.get("line_styles", ["--", "-", "x"])

  # Time
  time_data = {}
  for label, series_data in data.items():
    ts0 = series_data[key_time][0]
    time_data[label] = ts2sec(series_data[key_time].to_numpy() - ts0)

  # Plot subplots
  plt.figure()
  for i in range(3):
    plt.subplot(3, 1, i + 1)

    for (label, series_data), line in zip(data.items(), line_styles):
      line_style = colors[i] + line
      x_data = time_data[label]
      y_data = series_data[keys[i]].to_numpy()
      plt.plot(x_data, y_data, line_style, label=label)

    plt.xlabel("Time [s]")
    plt.ylabel(ylabel)
    plt.legend(loc=0)
    plt.title(f"{title} in {axis[i]}-axis")

  plt.subplots_adjust(hspace=0.65)


###############################################################################
# CV
###############################################################################

# UTILS #######################################################################


def lookat(cam_pos, target_pos, **kwargs):
  """ Form look at matrix """
  up_axis = kwargs.get('up_axis', np.array([0.0, -1.0, 0.0]))
  assert len(cam_pos) == 3
  assert len(target_pos) == 3
  assert len(up_axis) == 3

  # Note: If we were using OpenGL the cam_dir would be the opposite direction,
  # since in OpenGL the camera forward is -z. In robotics however our camera is
  # +z forward.
  cam_z = normalize(target_pos - cam_pos)
  cam_x = normalize(cross(up_axis, cam_z))
  cam_y = cross(cam_z, cam_x)

  T_WC = zeros((4, 4))
  T_WC[0:3, 0] = cam_x.T
  T_WC[0:3, 1] = cam_y.T
  T_WC[0:3, 2] = cam_z.T
  T_WC[0:3, 3] = cam_pos
  T_WC[3, 3] = 1.0

  return T_WC


# GEOMETRY ####################################################################


def linear_triangulation(P_i, P_j, z_i, z_j):
  """
  Linear triangulation

  This function is used to triangulate a single 3D point observed by two
  camera frames (be it in time with the same camera, or two different cameras
  with known extrinsics).

  Args:

    P_i (np.array): First camera 3x4 projection matrix
    P_j (np.array): Second camera 3x4 projection matrix
    z_i (np.array): First keypoint measurement
    z_j (np.array): Second keypoint measurement

  Returns:

    p_Ci (np.array): 3D point w.r.t first camera

  """

  # First three rows of P_i and P_j
  P1T_i = P_i[0, :]
  P2T_i = P_i[1, :]
  P3T_i = P_i[2, :]
  P1T_j = P_j[0, :]
  P2T_j = P_j[1, :]
  P3T_j = P_j[2, :]

  # Image point from the first and second frame
  x_i, y_i = z_i
  x_j, y_j = z_j

  # Form the A matrix of AX = 0
  A = zeros((4, 4))
  A[0, :] = x_i * P3T_i - P1T_i
  A[1, :] = y_i * P3T_i - P2T_i
  A[2, :] = x_j * P3T_j - P1T_j
  A[3, :] = y_j * P3T_j - P2T_j

  # Use SVD to solve AX = 0
  (_, _, Vh) = svd(A.T @ A)
  hp = Vh.T[:, -1]  # Get the best result from SVD (last column of V)
  hp = hp / hp[-1]  # Normalize the homogeneous 3D point
  p = hp[0:3]  # Return only the first three components (x, y, z)
  return p


# PINHOLE #####################################################################


def focal_length(image_width, fov_deg):
  """
  Estimated focal length based on `image_width` and field of fiew `fov_deg`
  in degrees.
  """
  return (image_width / 2.0) / tan(deg2rad(fov_deg / 2.0))


def pinhole_K(params):
  """ Form camera matrix K """
  fx, fy, cx, cy = params
  return np.array([[fx, 0.0, cx], [0.0, fy, cy], [0.0, 0.0, 1.0]])


def pinhole_P(params, T_WC):
  """ Form 3x4 projection matrix P """
  K = pinhole_K(params)
  T_CW = inv(T_WC)
  C = tf_rot(T_CW)
  r = tf_trans(T_CW)

  P = zeros((3, 4))
  P[0:3, 0:3] = C
  P[0:3, 3] = r
  P = K @ P
  return P


def pinhole_project(proj_params, p_C):
  """ Project 3D point onto image plane using pinhole camera model """
  assert len(proj_params) == 4
  assert len(p_C) == 3

  # Project
  x = np.array([p_C[0] / p_C[2], p_C[1] / p_C[2]])

  # Scale and center
  fx, fy, cx, cy = proj_params
  z = np.array([fx * x[0] + cx, fy * x[1] + cy])

  return z


def pinhole_params_jacobian(x):
  """ Form pinhole parameter jacobian """
  return np.array([[x[0], 0.0, 1.0, 0.0], [0.0, x[1], 0.0, 1.0]])


def pinhole_point_jacobian(proj_params):
  """ Form pinhole point jacobian """
  fx, fy, _, _ = proj_params
  return np.array([[fx, 0.0], [0.0, fy]])


# RADTAN4 #####################################################################


def radtan4_distort(dist_params, p):
  """ Distort point with Radial-Tangential distortion """
  assert len(dist_params) == 4
  assert len(p) == 2

  # Distortion parameters
  k1, k2, p1, p2 = dist_params

  # Point
  x, y = p

  # Apply radial distortion
  x2 = x * x
  y2 = y * y
  r2 = x2 + y2
  r4 = r2 * r2
  radial_factor = 1.0 + (k1 * r2) + (k2 * r4)
  x_dash = x * radial_factor
  y_dash = y * radial_factor

  # Apply tangential distortion
  xy = x * y
  x_ddash = x_dash + (2.0 * p1 * xy + p2 * (r2 + 2.0 * x2))
  y_ddash = y_dash + (p1 * (r2 + 2.0 * y2) + 2.0 * p2 * xy)
  return np.array([x_ddash, y_ddash])


def radtan4_point_jacobian(dist_params, p):
  """ Radial-tangential point jacobian """
  assert len(dist_params) == 4
  assert len(p) == 2

  # Distortion parameters
  k1, k2, p1, p2 = dist_params

  # Point
  x, y = p

  # Apply radial distortion
  x2 = x * x
  y2 = y * y
  r2 = x2 + y2
  r4 = r2 * r2

  # Point Jacobian
  # Let u = [x; y] normalized point
  # Let u' be the distorted u
  # The jacobian of u' w.r.t. u (or du'/du) is:
  J_point = zeros((2, 2))
  J_point[0, 0] = k1 * r2 + k2 * r4 + 2.0 * p1 * y + 6.0 * p2 * x
  J_point[0, 0] += x * (2.0 * k1 * x + 4.0 * k2 * x * r2) + 1.0
  J_point[1, 0] = 2.0 * p1 * x + 2.0 * p2 * y
  J_point[1, 0] += y * (2.0 * k1 * x + 4.0 * k2 * x * r2)
  J_point[0, 1] = J_point[1, 0]
  J_point[1, 1] = k1 * r2 + k2 * r4 + 6.0 * p1 * y + 2.0 * p2 * x
  J_point[1, 1] += y * (2.0 * k1 * y + 4.0 * k2 * y * r2) + 1.0
  # Above is generated using sympy

  return J_point


def radtan4_undistort(dist_params, p0):
  """ Un-distort point with Radial-Tangential distortion """
  assert len(dist_params) == 4
  assert len(p0) == 2

  # Undistort
  p = p0
  max_iter = 5

  for _ in range(max_iter):
    # Error
    p_distorted = radtan4_distort(dist_params, p)
    J = radtan4_point_jacobian(dist_params, p)
    err = (p0 - p_distorted)

    # Update
    # dp = inv(J' * J) * J' * err
    dp = pinv(J) @ err
    p = p + dp

    # Check threshold
    if (err.T @ err) < 1e-15:
      break

  return p


def radtan4_params_jacobian(dist_params, p):
  """ Radial-Tangential distortion parameter jacobian """
  assert len(dist_params) == 4
  assert len(p) == 2

  # Point
  x, y = p

  # Setup
  x2 = x * x
  y2 = y * y
  xy = x * y
  r2 = x2 + y2
  r4 = r2 * r2

  # Params Jacobian
  J_params = zeros((2, 4))
  J_params[0, 0] = x * r2
  J_params[0, 1] = x * r4
  J_params[0, 2] = 2.0 * xy
  J_params[0, 3] = 3.0 * x2 + y2
  J_params[1, 0] = y * r2
  J_params[1, 1] = y * r4
  J_params[1, 2] = x2 + 3.0 * y2
  J_params[1, 3] = 2.0 * xy

  return J_params


# EQUI4 #######################################################################


def equi4_distort(dist_params, p):
  """ Distort point with Equi-distant distortion """
  assert len(dist_params) == 4
  assert len(p) == 2

  # Distortion parameters
  k1, k2, k3, k4 = dist_params

  # Distort
  x, y = p
  r = sqrt(x * x + y * y)
  th = math.atan(r)
  th2 = th * th
  th4 = th2 * th2
  th6 = th4 * th2
  th8 = th4 * th4
  thd = th * (1.0 + k1 * th2 + k2 * th4 + k3 * th6 + k4 * th8)
  s = thd / r
  x_dash = s * x
  y_dash = s * y
  return np.array([x_dash, y_dash])


def equi4_undistort(dist_params, p):
  """ Undistort point using Equi-distant distortion """
  thd = sqrt(p(0) * p(0) + p[0] * p[0])

  # Distortion parameters
  k1, k2, k3, k4 = dist_params

  th = thd  # Initial guess
  for _ in range(20):
    th2 = th * th
    th4 = th2 * th2
    th6 = th4 * th2
    th8 = th4 * th4
    th = thd / (1.0 + k1 * th2 + k2 * th4 + k3 * th6 + k4 * th8)

  scaling = tan(th) / thd
  return np.array([p[0] * scaling, p[1] * scaling])


def equi4_params_jacobian(dist_params, p):
  """ Equi-distant distortion params jacobian """
  assert len(dist_params) == 4
  assert len(p) == 2

  # Jacobian
  x, y = p
  r = sqrt(x**2 + y**2)
  th = atan(r)

  J_params = zeros((2, 4))
  J_params[0, 0] = x * th**3 / r
  J_params[0, 1] = x * th**5 / r
  J_params[0, 2] = x * th**7 / r
  J_params[0, 3] = x * th**9 / r

  J_params[1, 0] = y * th**3 / r
  J_params[1, 1] = y * th**5 / r
  J_params[1, 2] = y * th**7 / r
  J_params[1, 3] = y * th**9 / r

  return J_params


def equi4_point_jacobian(dist_params, p):
  """ Equi-distant distortion point jacobian """
  assert len(dist_params) == 4
  assert len(p) == 2

  # Distortion parameters
  k1, k2, k3, k4 = dist_params

  # Jacobian
  x, y = p
  r = sqrt(x**2 + y**2)

  th = math.atan(r)
  th2 = th**2
  th4 = th**4
  th6 = th**6
  th8 = th**8
  thd = th * (1.0 + k1 * th2 + k2 * th4 + k3 * th6 + k4 * th8)

  th_r = 1.0 / (r * r + 1.0)
  thd_th = 1.0 + 3.0 * k1 * th2
  thd_th += 5.0 * k2 * th4
  thd_th += 7.0 * k3 * th6
  thd_th += 9.0 * k4 * th8
  s = thd / r
  s_r = thd_th * th_r / r - thd / (r * r)
  r_x = 1.0 / r * x
  r_y = 1.0 / r * y

  J_point = zeros((2, 2))
  J_point[0, 0] = s + x * s_r * r_x
  J_point[0, 1] = x * s_r * r_y
  J_point[1, 0] = y * s_r * r_x
  J_point[1, 1] = s + y * s_r * r_y

  return J_point


# PINHOLE RADTAN4 #############################################################


def pinhole_radtan4_project(proj_params, dist_params, p_C):
  """ Pinhole + Radial-Tangential project """
  assert len(proj_params) == 4
  assert len(dist_params) == 4
  assert len(p_C) == 3

  # Project
  x = np.array([p_C[0] / p_C[2], p_C[1] / p_C[2]])

  # Distort
  x_dist = radtan4_distort(dist_params, x)

  # Scale and center to image plane
  fx, fy, cx, cy = proj_params
  z = np.array([fx * x_dist[0] + cx, fy * x_dist[1] + cy])
  return z


def pinhole_radtan4_backproject(proj_params, dist_params, z):
  """ Pinhole + Radial-Tangential back-project """
  assert len(proj_params) == 4
  assert len(dist_params) == 4
  assert len(z) == 2

  # Convert image pixel coordinates to normalized retinal coordintes
  fx, fy, cx, cy = proj_params
  x = np.array([(z[0] - cx) / fx, (z[1] - cy) / fy, 1.0])

  # Undistort
  x = radtan4_undistort(dist_params, x)

  # 3D ray
  p = np.array([x[0], x[1], 1.0])
  return p


def pinhole_radtan4_undistort(proj_params, dist_params, z):
  """ Pinhole + Radial-Tangential undistort """
  assert len(proj_params) == 4
  assert len(dist_params) == 4
  assert len(z) == 2

  # Back project and undistort
  fx, fy, cx, cy = proj_params
  p = np.array([(z[0] - cx) / fx, (z[1] - cy) / fy])
  p_undist = radtan4_undistort(dist_params, p)

  # Project undistorted point to image plane
  return np.array([p_undist[0] * fx + cx, p_undist[1] * fy + cy])


def pinhole_radtan4_project_jacobian(proj_params, dist_params, p_C):
  """ Pinhole + Radial-Tangential project jacobian """
  assert len(proj_params) == 4
  assert len(dist_params) == 4
  assert len(p_C) == 3

  # Project 3D point
  x = np.array([p_C[0] / p_C[2], p_C[1] / p_C[2]])

  # Jacobian
  J_proj = zeros((2, 3))
  J_proj[0, :] = [1 / p_C[2], 0, -p_C[0] / p_C[2]**2]
  J_proj[1, :] = [0, 1 / p_C[2], -p_C[1] / p_C[2]**2]
  J_dist_point = radtan4_point_jacobian(dist_params, x)
  J_proj_point = pinhole_point_jacobian(proj_params)

  return J_proj_point @ J_dist_point @ J_proj


def pinhole_radtan4_params_jacobian(proj_params, dist_params, p_C):
  """ Pinhole + Radial-Tangential params jacobian """
  assert len(proj_params) == 4
  assert len(dist_params) == 4
  assert len(p_C) == 3

  x = np.array([p_C[0] / p_C[2], p_C[1] / p_C[2]])  # Project 3D point
  x_dist = radtan4_distort(dist_params, x)  # Distort point

  J_proj_point = pinhole_point_jacobian(proj_params)
  J_dist_params = radtan4_params_jacobian(dist_params, x)

  J = zeros((2, 8))
  J[0:2, 0:4] = pinhole_params_jacobian(x_dist)
  J[0:2, 4:8] = J_proj_point @ J_dist_params
  return J


# PINHOLE EQUI4 ###############################################################


def pinhole_equi4_project(proj_params, dist_params, p_C):
  """ Pinhole + Equi-distant project """
  assert len(proj_params) == 4
  assert len(dist_params) == 4
  assert len(p_C) == 3

  # Project
  x = np.array([p_C[0] / p_C[2], p_C[1] / p_C[2]])

  # Distort
  x_dist = equi4_distort(dist_params, x)

  # Scale and center to image plane
  fx, fy, cx, cy = proj_params
  z = np.array([fx * x_dist[0] + cx, fy * x_dist[1] + cy])
  return z


def pinhole_equi4_backproject(proj_params, dist_params, z):
  """ Pinhole + Equi-distant back-project """
  assert len(proj_params) == 4
  assert len(dist_params) == 4
  assert len(z) == 2

  # Convert image pixel coordinates to normalized retinal coordintes
  fx, fy, cx, cy = proj_params
  x = np.array([(z[0] - cx) / fx, (z[1] - cy) / fy, 1.0])

  # Undistort
  x = equi4_undistort(dist_params, x)

  # 3D ray
  p = np.array([x[0], x[1], 1.0])
  return p


def pinhole_equi4_undistort(proj_params, dist_params, z):
  """ Pinhole + Equi-distant undistort """
  assert len(proj_params) == 4
  assert len(dist_params) == 4
  assert len(z) == 2

  # Back project and undistort
  fx, fy, cx, cy = proj_params
  p = np.array([(z[0] - cx) / fx, (z[1] - cy) / fy])
  p_undist = equi4_undistort(dist_params, p)

  # Project undistorted point to image plane
  return np.array([p_undist[0] * fx + cx, p_undist[1] * fy + cy])


def pinhole_equi4_project_jacobian(proj_params, dist_params, p_C):
  """ Pinhole + Equi-distant project jacobian """
  assert len(proj_params) == 4
  assert len(dist_params) == 4
  assert len(p_C) == 3

  # Project 3D point
  x = np.array([p_C[0] / p_C[2], p_C[1] / p_C[2]])

  # Jacobian
  J_proj = zeros((2, 3))
  J_proj[0, :] = [1 / p_C[2], 0, -p_C[0] / p_C[2]**2]
  J_proj[1, :] = [0, 1 / p_C[2], -p_C[1] / p_C[2]**2]
  J_dist_point = equi4_point_jacobian(dist_params, x)
  J_proj_point = pinhole_point_jacobian(proj_params)
  return J_proj_point @ J_dist_point @ J_proj


def pinhole_equi4_params_jacobian(proj_params, dist_params, p_C):
  """ Pinhole + Equi-distant params jacobian """
  assert len(proj_params) == 4
  assert len(dist_params) == 4
  assert len(p_C) == 3

  x = np.array([p_C[0] / p_C[2], p_C[1] / p_C[2]])  # Project 3D point
  x_dist = equi4_distort(dist_params, x)  # Distort point

  J_proj_point = pinhole_point_jacobian(proj_params)
  J_dist_params = equi4_params_jacobian(dist_params, x)

  J = zeros((2, 8))
  J[0:2, 0:4] = pinhole_params_jacobian(x_dist)
  J[0:2, 4:8] = J_proj_point @ J_dist_params
  return J


# CAMERA GEOMETRY #############################################################


@dataclass
class CameraGeometry:
  """ Camera Geometry """
  cam_idx: int
  resolution: tuple
  proj_model: str
  dist_model: str
  proj_params_size: int
  dist_params_size: int

  project_fn: FunctionType
  backproject_fn: FunctionType
  undistort_fn: FunctionType
  J_proj_fn: FunctionType
  J_params_fn: FunctionType

  def get_proj_params_size(self):
    """ Return projection parameter size """
    return self.proj_params_size

  def get_dist_params_size(self):
    """ Return distortion parameter size """
    return self.dist_params_size

  def get_params_size(self):
    """ Return parameter size """
    return self.get_proj_params_size() + self.get_dist_params_size()

  def proj_params(self, params):
    """ Extract projection parameters """
    return params[:self.proj_params_size]

  def dist_params(self, params):
    """ Extract distortion parameters """
    return params[-self.dist_params_size:]

  def project(self, params, p_C):
    """ Project point `p_C` with camera parameters `params` """
    # Project
    proj_params = params[:self.proj_params_size]
    dist_params = params[-self.dist_params_size:]
    z = self.project_fn(proj_params, dist_params, p_C)

    # Make sure point is infront of camera
    if p_C[2] < 0.0:
      return False, z

    # Make sure image point is within image bounds
    x_ok = z[0] >= 0.0 and z[0] <= self.resolution[0]
    y_ok = z[1] >= 0.0 and z[1] <= self.resolution[1]
    if x_ok and y_ok:
      return True, z

    return False, z

  def backproject(self, params, z):
    """ Back-project image point `z` with camera parameters `params` """
    proj_params = params[:self.proj_params_size]
    dist_params = params[-self.dist_params_size:]
    return self.project_fn(proj_params, dist_params, z)

  def undistort(self, params, z):
    """ Undistort image point `z` with camera parameters `params` """
    proj_params = params[:self.proj_params_size]
    dist_params = params[-self.dist_params_size:]
    return self.undistort_fn(proj_params, dist_params, z)

  def J_proj(self, params, p_C):
    """ Form Jacobian w.r.t. p_C """
    proj_params = params[:self.proj_params_size]
    dist_params = params[-self.dist_params_size:]
    return self.J_proj_fn(proj_params, dist_params, p_C)

  def J_params(self, params, p_C):
    """ Form Jacobian w.r.t. camera parameters """
    proj_params = params[:self.proj_params_size]
    dist_params = params[-self.dist_params_size:]
    return self.J_params_fn(proj_params, dist_params, p_C)


def pinhole_radtan4_setup(cam_idx, cam_res):
  """ Setup Pinhole + Radtan4 camera geometry """
  return CameraGeometry(cam_idx, cam_res, "pinhole", "radtan4", 4, 4,
                        pinhole_radtan4_project, pinhole_radtan4_backproject,
                        pinhole_radtan4_undistort,
                        pinhole_radtan4_project_jacobian,
                        pinhole_radtan4_params_jacobian)


def pinhole_equi4_setup(cam_idx, cam_res):
  """ Setup Pinhole + Equi camera geometry """
  return CameraGeometry(cam_idx, cam_res, "pinhole", "equi4", 4, 4,
                        pinhole_equi4_project, pinhole_equi4_backproject,
                        pinhole_equi4_undistort, pinhole_equi4_project_jacobian,
                        pinhole_equi4_params_jacobian)


def camera_geometry_setup(cam_idx, cam_res, proj_model, dist_model):
  """ Setup camera geometry """
  if proj_model == "pinhole" and dist_model == "radtan4":
    return pinhole_radtan4_setup(cam_idx, cam_res)
  elif proj_model == "pinhole" and dist_model == "equi4":
    return pinhole_equi4_setup(cam_idx, cam_res)
  else:
    raise RuntimeError(f"Unrecognized [{proj_model}]-[{dist_model}] combo!")


################################################################################
# DATASET
################################################################################

# TIMELINE######################################################################


@dataclass
class CameraEvent:
  """ Camera Event """
  ts: int
  cam_idx: int
  image: np.array


@dataclass
class ImuEvent:
  """ IMU Event """
  ts: int
  imu_idx: int
  acc: np.array
  gyr: np.array


@dataclass
class Timeline:
  """ Timeline """
  def __init__(self):
    self.data = {}

  def num_timestamps(self):
    """ Return number of timestamps """
    return len(self.data)

  def num_events(self):
    """ Return number of events """
    nb_events = 0
    for _, events in self.data:
      nb_events += len(events)
    return nb_events

  def get_timestamps(self):
    """ Get timestamps """
    return sorted(list(self.data.keys()))

  def add_event(self, ts, event):
    """ Add event """
    if ts not in self.data:
      self.data[ts] = [event]
    else:
      self.data[ts].append(event)

  def get_events(self, ts):
    """ Get events """
    return self.data[ts]


# EUROC ########################################################################


class EurocSensor:
  """ Euroc Sensor """
  def __init__(self, yaml_path):
    # Load yaml file
    config = load_yaml(yaml_path)

    # General sensor definitions.
    self.sensor_type = config.sensor_type
    self.comment = config.comment

    # Sensor extrinsics wrt. the body-frame.
    self.T_BS = np.array(config.T_BS.data).reshape((4, 4))

    # Camera specific definitions.
    if config.sensor_type == "camera":
      self.rate_hz = config.rate_hz
      self.resolution = config.resolution
      self.camera_model = config.camera_model
      self.intrinsics = config.intrinsics
      self.distortion_model = config.distortion_model
      self.distortion_coefficients = config.distortion_coefficients

    elif config.sensor_type == "imu":
      self.rate_hz = config.rate_hz
      self.gyro_noise_density = config.gyroscope_noise_density
      self.gyro_random_walk = config.gyroscope_random_walk
      self.accel_noise_density = config.accelerometer_noise_density
      self.accel_random_walk = config.accelerometer_random_walk


class EurocImuData:
  """ Euroc Imu data """
  def __init__(self, data_dir):
    self.imu_dir = Path(data_dir, 'mav0', 'imu0')
    self.config = EurocSensor(Path(self.imu_dir, 'sensor.yaml'))
    self.timestamps = []
    self.acc = {}
    self.gyr = {}

    # Load data
    df = pandas.read_csv(Path(self.imu_dir, 'data.csv'))
    df = df.rename(columns=lambda x: x.strip())

    # -- Timestamp
    timestamps = df['#timestamp [ns]'].to_numpy()
    # -- Accelerometer measurement
    acc_x = df['a_RS_S_x [m s^-2]'].to_numpy()
    acc_y = df['a_RS_S_y [m s^-2]'].to_numpy()
    acc_z = df['a_RS_S_z [m s^-2]'].to_numpy()
    # -- Gyroscope measurement
    gyr_x = df['w_RS_S_x [rad s^-1]'].to_numpy()
    gyr_y = df['w_RS_S_y [rad s^-1]'].to_numpy()
    gyr_z = df['w_RS_S_z [rad s^-1]'].to_numpy()
    # -- Load
    for i, ts in enumerate(timestamps):
      self.timestamps.append(ts)
      self.acc[ts] = np.array([acc_x[i], acc_y[i], acc_z[i]])
      self.gyr[ts] = np.array([gyr_x[i], gyr_y[i], gyr_z[i]])


class EurocCameraData:
  """ Euroc Camera data """
  def __init__(self, data_dir, cam_idx):
    self.cam_idx = cam_idx
    self.cam_dir = Path(data_dir, 'mav0', 'cam' + str(cam_idx))
    self.config = EurocSensor(Path(self.cam_dir, 'sensor.yaml'))
    self.timestamps = []
    self.image_paths = {}

    # Load image paths
    cam_data_dir = str(Path(self.cam_dir, 'data', '*.png'))
    for img_file in sorted(glob.glob(cam_data_dir)):
      ts_str, _ = os.path.basename(img_file).split('.')
      ts = int(ts_str)
      self.timestamps.append(ts)
      self.image_paths[ts] = img_file

  def get_image_path_list(self):
    """ Return list of image paths """
    return [img_path for _, img_path in self.image_paths]


class EurocGroundTruth:
  """ Euroc ground truth """
  def __init__(self, data_dir):
    self.timestamps = []
    self.T_WB = {}
    self.v_WB = {}
    self.w_WB = {}
    self.a_WB = {}

    # Load data
    dir_name = 'state_groundtruth_estimate0'
    data_csv = Path(data_dir, 'mav0', dir_name, 'data.csv')
    df = pandas.read_csv(data_csv)
    df = df.rename(columns=lambda x: x.strip())
    # -- Timestamp
    timestamps = df['#timestamp'].to_numpy()
    # -- Body pose in world frame
    rx_list = df['p_RS_R_x [m]'].to_numpy()
    ry_list = df['p_RS_R_y [m]'].to_numpy()
    rz_list = df['p_RS_R_z [m]'].to_numpy()
    qw_list = df['q_RS_w []'].to_numpy()
    qx_list = df['q_RS_x []'].to_numpy()
    qy_list = df['q_RS_y []'].to_numpy()
    qz_list = df['q_RS_z []'].to_numpy()
    # -- Body velocity in world frame
    vx_list = df['v_RS_R_x [m s^-1]'].to_numpy()
    vy_list = df['v_RS_R_y [m s^-1]'].to_numpy()
    vz_list = df['v_RS_R_z [m s^-1]'].to_numpy()
    # -- Add to class
    for i, ts in enumerate(timestamps):
      r_WB = np.array([rx_list[i], ry_list[i], rz_list[i]])
      q_WB = np.array([qw_list[i], qx_list[i], qy_list[i], qz_list[i]])
      v_WB = np.array([vx_list[i], vy_list[i], vz_list[i]])

      self.timestamps.append(ts)
      self.T_WB[ts] = tf(q_WB, r_WB)
      self.v_WB[ts] = v_WB


class EurocDataset:
  """ Euroc Dataset """
  def __init__(self, data_path):
    # Data path
    self.data_path = data_path
    if os.path.isdir(data_path) is False:
      raise RuntimeError(f"Path {data_path} does not exist!")

    # Data
    self.imu0_data = EurocImuData(self.data_path)
    self.cam0_data = EurocCameraData(self.data_path, 0)
    self.cam1_data = EurocCameraData(self.data_path, 1)
    self.ground_truth = EurocGroundTruth(self.data_path)
    self.timeline = self._form_timeline()

  def _form_timeline(self):
    timeline = Timeline()

    # Form timeline
    # -- Add imu0 events
    for ts in self.imu0_data.timestamps:
      acc = self.imu0_data.acc[ts]
      gyr = self.imu0_data.gyr[ts]
      timeline.add_event(ts, ImuEvent(ts, 0, acc, gyr))

    # -- Add cam0 events
    for ts, img_path in self.cam0_data.image_paths.items():
      timeline.add_event(ts, CameraEvent(ts, 0, img_path))

    # -- Add cam1 events
    for ts, img_path in self.cam1_data.image_paths.items():
      timeline.add_event(ts, CameraEvent(ts, 1, img_path))

    return timeline

  def get_camera_image(self, cam_idx, ts):
    """ Get camera image """
    img_path = None
    if cam_idx == 0:
      img_path = self.cam0_data.image_paths[ts]
    elif cam_idx == 1:
      img_path = self.cam1_data.image_paths[ts]
    else:
      raise RuntimeError("cam_idx has to be 0 or 1")
    return cv2.imread(img_path, cv2.IMREAD_GRAYSCALE)

  def get_ground_truth_pose(self, ts):
    """ Get ground truth pose T_WB at timestamp `ts` """
    # Pre-check
    if ts <= self.ground_truth.timestamps[0]:
      return None
    elif ts >= self.ground_truth.timestamps[-1]:
      return None

    # Loop throught timestamps
    for k, ground_truth_ts in enumerate(self.ground_truth.timestamps):
      if ts == ground_truth_ts:
        return self.ground_truth.T_WB[ts]
      elif self.ground_truth.timestamps[k] > ts:
        ts_i = self.ground_truth.timestamps[k - 1]
        ts_j = self.ground_truth.timestamps[k]
        alpha = float(ts_j - ts) / float(ts_j - ts_i)
        pose_i = self.ground_truth.T_WB[ts_i]
        pose_j = self.ground_truth.T_WB[ts_j]
        return tf_lerp(pose_i, pose_j, alpha)

    return None


# KITTI #######################################################################


class KittiCameraData:
  """ KittiCameraDataset """
  def __init__(self, cam_idx, seq_path):
    self.cam_idx = cam_idx
    self.seq_path = seq_path
    self.cam_path = Path(self.seq_path, "image_" + str(self.cam_idx).zfill(2))
    self.img_dir = Path(self.cam_path, "data")
    self.img_paths = sorted(glob.glob(str(Path(self.img_dir, "*.png"))))


class KittiRawDataset:
  """ KittiRawDataset """
  def __init__(self, data_dir, date, seq, is_sync):
    # Paths
    self.data_dir = data_dir
    self.date = date
    self.seq = seq.zfill(4)
    self.sync = "sync" if is_sync else "extract"
    self.seq_name = "_".join([self.date, "drive", self.seq, self.sync])
    self.seq_path = Path(self.data_dir, self.date, self.seq_name)

    # Camera data
    self.cam0_data = KittiCameraData(0, self.seq_path)
    self.cam1_data = KittiCameraData(1, self.seq_path)
    self.cam2_data = KittiCameraData(2, self.seq_path)
    self.cam3_data = KittiCameraData(3, self.seq_path)

    # Calibration
    calib_cam_to_cam_filepath = Path(self.data_dir, "calib_cam_to_cam.txt")
    calib_imu_to_velo_filepath = Path(self.data_dir, "calib_imu_to_velo.txt")
    calib_velo_to_cam_filepath = Path(self.data_dir, "calib_velo_to_cam.txt")
    self.calib_cam_to_cam = self._read_calib_file(calib_cam_to_cam_filepath)
    self.calib_imu_to_velo = self._read_calib_file(calib_imu_to_velo_filepath)
    self.calib_velo_to_cam = self._read_calib_file(calib_velo_to_cam_filepath)

  @classmethod
  def _read_calib_file(cls, fp):
    data = {}
    with open(fp, 'r') as f:
      for line in f.readlines():
        key, value = line.split(':', 1)
        # The only non-float values in these files are dates, which
        # we don't care about anyway
        try:
          data[key] = np.array([float(x) for x in value.split()])
        except ValueError:
          pass
      return data

  def nb_camera_images(self, cam_idx=0):
    """ Return number of camera images """
    assert cam_idx >= 0 and cam_idx <= 3
    if cam_idx == 0:
      return len(self.cam0_data.img_paths)
    elif cam_idx == 1:
      return len(self.cam1_data.img_paths)
    elif cam_idx == 2:
      return len(self.cam2_data.img_paths)
    elif cam_idx == 3:
      return len(self.cam3_data.img_paths)

    return None

  def get_velodyne_extrinsics(self):
    """ Get velodyne extrinsics """
    # Form imu-velo extrinsics T_BV
    C_VB = self.calib_imu_to_velo['R'].reshape((3, 3))
    r_VB = self.calib_imu_to_velo['T']
    T_VB = tf(C_VB, r_VB)
    T_BV = inv(T_VB)
    return T_BV

  def get_camera_extrinsics(self, cam_idx):
    """ Get camera extrinsics T_BCi """
    # Form imu-velo extrinsics T_VB
    C_VB = self.calib_imu_to_velo['R'].reshape((3, 3))
    r_VB = self.calib_imu_to_velo['T']
    T_VB = tf(C_VB, r_VB)

    # Form velo-cam extrinsics T_C0V
    C_C0V = self.calib_velo_to_cam['R'].reshape((3, 3))
    r_C0V = self.calib_velo_to_cam['T']
    T_C0V = tf(C_C0V, r_C0V)

    # Form cam-cam extrinsics T_CiC0
    cam_str = str(cam_idx)
    C_CiC0 = self.calib_cam_to_cam['R_' + cam_str.zfill(2)].reshape((3, 3))
    r_CiC0 = self.calib_cam_to_cam['T_' + cam_str.zfill(2)]
    T_CiC0 = tf(C_CiC0, r_CiC0)

    # Form camera extrinsics T_BC0
    T_CiB = T_CiC0 @ T_C0V @ T_VB
    T_BCi = inv(T_CiB)

    return T_BCi

  def get_camera_image(self, cam_idx, **kwargs):
    """ Get camera image """
    assert cam_idx >= 0 and cam_idx <= 3
    imread_flag = kwargs.get('imread_flag', cv2.IMREAD_GRAYSCALE)
    img_idx = kwargs['index']

    if cam_idx == 0:
      return cv2.imread(self.cam0_data.img_paths[img_idx], imread_flag)
    elif cam_idx == 1:
      return cv2.imread(self.cam1_data.img_paths[img_idx], imread_flag)
    elif cam_idx == 2:
      return cv2.imread(self.cam2_data.img_paths[img_idx], imread_flag)
    elif cam_idx == 3:
      return cv2.imread(self.cam3_data.img_paths[img_idx], imread_flag)

    return None

  def plot_frames(self):
    """ Plot Frames """
    T_BV = self.get_velodyne_extrinsics()
    T_BC0 = self.get_camera_extrinsics(0)
    T_BC1 = self.get_camera_extrinsics(1)
    T_BC2 = self.get_camera_extrinsics(2)
    T_BC3 = self.get_camera_extrinsics(3)

    plt.figure()
    ax = plt.axes(projection='3d')
    plot_tf(ax, eye(4), size=0.1, name="imu")
    plot_tf(ax, T_BV, size=0.1, name="velo")
    plot_tf(ax, T_BC0, size=0.1, name="cam0")
    plot_tf(ax, T_BC1, size=0.1, name="cam1")
    plot_tf(ax, T_BC2, size=0.1, name="cam2")
    plot_tf(ax, T_BC3, size=0.1, name="cam3")
    ax.set_xlabel("x [m]")
    ax.set_ylabel("y [m]")
    ax.set_zlabel("z [m]")
    plot_set_axes_equal(ax)
    plt.show()


###############################################################################
# FILTER
###############################################################################


def compl_filter(gyro, accel, dt, roll, pitch):
  """
  A simple complementary filter that uses `gyro` and `accel` measurements to
  estimate the attitude in `roll` and `pitch`. Where `dt` is the update
  rate of the `gyro` measurements in seconds.
  """
  # Calculate pitch and roll using gyroscope
  wx, wy, _ = gyro
  gyro_roll = (wx * dt) + roll
  gyro_pitch = (wy * dt) + pitch

  # Calculate pitch and roll using accelerometer
  ax, ay, az = accel
  accel_roll = (atan(ay / sqrt(ax * ay + az * az))) * 180.0 / pi
  accel_pitch = (atan(ax / sqrt(ay * ay + az * az))) * 180.0 / pi

  # Complimentary filter
  pitch = (0.98 * gyro_pitch) + (0.02 * accel_pitch)
  roll = (0.98 * gyro_roll) + (0.02 * accel_roll)

  return (roll, pitch)


###############################################################################
# STATE ESTIMATION
###############################################################################

# STATE VARIABLES #############################################################


@dataclass
class StateVariable:
  """ State variable """
  ts: int
  var_type: str
  param: np.array
  parameterization: str
  min_dims: int
  fix: bool
  data: Optional[dict] = None
  param_id: int = None
  marginalize: bool = False

  def set_param_id(self, pid):
    """ Set parameter id """
    self.param_id = pid


class StateVariableType(Enum):
  """ State Variable Type """
  POSE = 1
  EXTRINSICS = 2
  FEATURE = 3
  CAMERA = 4
  SPEED_AND_BIASES = 5


class FeatureMeasurements:
  """ Feature measurements """
  def __init__(self):
    self._init = False
    self._data = {}

  def initialized(self):
    """ Check if feature is initialized """
    return self._init

  def has_overlap(self, ts):
    """ Check if feature has overlap at timestamp `ts` """
    return len(self._data[ts]) > 1

  def set_initialized(self):
    """ Set feature as initialized """
    self._init = True

  def update(self, ts, cam_idx, z):
    """ Add feature measurement """
    assert len(z) == 2
    if ts not in self._data:
      self._data[ts] = {}
    self._data[ts][cam_idx] = z

  def get(self, ts, cam_idx):
    """ Get feature measurement """
    return self._data[ts][cam_idx]

  def get_overlaps(self, ts):
    """ Get feature overlaps """
    overlaps = []
    for cam_idx, z in self._data[ts].items():
      overlaps.append((cam_idx, z))
    return overlaps


def tf2pose(T):
  """ Form pose vector """
  rx, ry, rz = tf_trans(T)
  qw, qx, qy, qz = tf_quat(T)
  return np.array([rx, ry, rz, qx, qy, qz, qw])


def pose2tf(pose_vec):
  """ Convert pose vector to transformation matrix """
  rx, ry, rz = pose_vec[0:3]
  qx, qy, qz, qw = pose_vec[3:7]
  return tf(np.array([qw, qx, qy, qz]), np.array([rx, ry, rz]))


def pose_setup(ts, param, **kwargs):
  """ Form pose state-variable """
  fix = kwargs.get('fix', False)
  param = tf2pose(param) if param.shape == (4, 4) else param
  return StateVariable(ts, "pose", param, None, 6, fix)


def extrinsics_setup(param, **kwargs):
  """ Form extrinsics state-variable """
  fix = kwargs.get('fix', False)
  param = tf2pose(param) if param.shape == (4, 4) else param
  return StateVariable(None, "extrinsics", param, None, 6, fix)


def camera_params_setup(cam_idx, res, proj_model, dist_model, param, **kwargs):
  """ Form camera parameters state-variable """
  fix = kwargs.get('fix', False)
  data = camera_geometry_setup(cam_idx, res, proj_model, dist_model)
  return StateVariable(None, "camera", param, None, len(param), fix, data)


def feature_setup(param, **kwargs):
  """ Form feature state-variable """
  fix = kwargs.get('fix', False)
  data = FeatureMeasurements()
  return StateVariable(None, "feature", param, None, len(param), fix, data)


def speed_biases_setup(ts, vel, ba, bg, **kwargs):
  """ Form speed and biases state-variable """
  fix = kwargs.get('fix', False)
  param = np.block([vel, ba, bg])
  return StateVariable(ts, "speed_and_biases", param, None, len(param), fix)


def perturb_state_variable(sv, i, step_size):
  """ Perturb state variable """
  if sv.var_type == "pose" or sv.var_type == "extrinsics":
    T = pose2tf(sv.param)
    T_dash = tf_perturb(T, i, step_size)
    sv.param = tf2pose(T_dash)
  else:
    sv.param[i] += step_size

  return sv


def update_state_variable(sv, dx):
  """ Update state variable """
  if sv.var_type == "pose" or sv.var_type == "extrinsics":
    T = pose2tf(sv.param)
    T_prime = tf_update(T, dx)
    sv.param = tf2pose(T_prime)
  else:
    sv.param += dx


def idp_param(cam_params, T_WC, z):
  """ Create inverse depth parameter """
  # Back project image pixel measurmeent to 3D ray
  cam_geom = cam_params.data
  x = cam_geom.backproject(cam_params.params, z)

  # Convert 3D ray from camera frame to world frame
  r_WC = tf_trans(T_WC)
  C_WC = tf_rot(T_WC)
  h_W = C_WC @ x

  # Obtain bearing (theta, phi) and inverse depth (rho)
  theta = atan2(h_W[0], h_W[2])
  phi = atan2(-h_W[1], sqrt(h_W[0] * h_W(1) + h_W[2] * h_W[2]))
  rho = 0.1
  # sigma_rho = 0.5  # variance of inverse depth

  # Form inverse depth parameter
  param = np.array([r_WC, theta, phi, rho])
  return param


def idp_param_jacobian(param):
  """ Inverse depth parameter jacobian """
  _, _, _, theta, phi, rho = param
  p_W = np.array([cos(phi) * sin(theta), -sin(phi), cos(phi) * cos(theta)])

  J_x = np.array([1.0, 0.0, 0.0])
  J_y = np.array([0.0, 1.0, 0.0])
  J_z = np.array([0.0, 0.0, 1.0])
  J_theta = 1.0 / rho * np.array(
      [cos(phi) * cos(theta), 0.0,
       cos(phi) * -sin(theta)])
  J_phi = 1.0 / rho * np.array(
      [-sin(phi) * sin(theta), -cos(phi), -sin(phi) * cos(theta)])
  J_rho = -1.0 / rho**2 @ p_W
  J_param = np.block([J_x, J_y, J_z, J_theta, J_phi, J_rho])
  return J_param


def idp_point(param):
  """ Inverse depth parmaeter to point """
  # Extract parameter values
  x, y, z, theta, phi, rho = param

  # Camera position in world frame
  r_WC = np.array([x, y, z])

  # Convert bearing to 3D ray from camera frame
  m = np.array([cos(phi) * sin(theta), -sin(phi), cos(phi) * cos(theta)])

  # Form 3D point in world frame
  p_W = r_WC + (1.0 / rho) @ m

  return p_W


# FACTORS ######################################################################


class Factor:
  """ Factor """
  def __init__(self, ftype, pids, z, covar):
    self.factor_id = None
    self.factor_type = ftype
    self.param_ids = pids
    self.measurement = z
    self.covar = covar
    self.sqrt_info = chol(inv(self.covar)).T

  def set_factor_id(self, fid):
    """ Set factor id """
    self.factor_id = fid

  def check_jacobian(self, fvars, var_idx, jac_name, **kwargs):
    """ Check factor jacobian """

    # Step size and threshold
    h = kwargs.get('step_size', 1e-8)
    threshold = kwargs.get('threshold', 1e-4)
    verbose = kwargs.get('verbose', False)

    # Calculate baseline
    params = [sv.param for sv in fvars]
    r, jacs = self.eval(params)

    # Numerical diff
    J_fdiff = zeros((len(r), fvars[var_idx].min_dims))
    for i in range(fvars[var_idx].min_dims):
      # Forward difference and evaluate
      vars_fwd = copy.deepcopy(fvars)
      vars_fwd[var_idx] = perturb_state_variable(vars_fwd[var_idx], i, 0.5 * h)
      r_fwd, _ = self.eval([sv.param for sv in vars_fwd])

      # Backward difference and evaluate
      vars_bwd = copy.deepcopy(fvars)
      vars_bwd[var_idx] = perturb_state_variable(vars_bwd[var_idx], i, -0.5 * h)
      r_bwd, _ = self.eval([sv.param for sv in vars_bwd])

      # Central finite difference
      J_fdiff[:, i] = (r_fwd - r_bwd) / h

    J = jacs[var_idx]
    return check_jacobian(jac_name, J_fdiff, J, threshold, verbose)


class PoseFactor(Factor):
  """ Pose Factor """
  def __init__(self, pids, z, covar):
    assert len(pids) == 1
    assert z.shape == (4, 4)
    assert covar.shape == (6, 6)
    Factor.__init__(self, "PoseFactor", pids, z, covar)

  def eval(self, params, **kwargs):
    """ Evaluate """
    assert len(params) == 1
    assert len(params[0]) == 7

    # Measured pose
    T_meas = self.measurement
    q_meas = tf_quat(T_meas)
    r_meas = tf_trans(T_meas)

    # Estimated pose
    T_est = pose2tf(params[0])
    q_est = tf_quat(T_est)
    r_est = tf_trans(T_est)

    # Form residuals (pose - pose_est)
    dr = r_meas - r_est
    dq = quat_mul(quat_inv(q_meas), q_est)
    dtheta = 2 * dq[1:4]
    r = self.sqrt_info @ np.block([dr, dtheta])
    if kwargs.get('only_residuals', False):
      return r

    # Form jacobians
    J = zeros((6, 6))
    J[0:3, 0:3] = -eye(3)
    J[3:6, 3:6] = quat_left(dq)[1:4, 1:4]
    J = self.sqrt_info @ J

    return (r, [J])


class MultiCameraBuffer:
  """ Multi-camera buffer """
  def __init__(self, nb_cams=0):
    self.nb_cams = nb_cams
    self._ts = []
    self._data = {}

  def reset(self):
    """ Reset buffer """
    self._ts = []
    self._data = {}

  def add(self, ts, cam_idx, data):
    """ Add camera event """
    if self.nb_cams == 0:
      raise RuntimeError("MulitCameraBuffer not initialized yet!")

    self._ts.append(ts)
    self._data[cam_idx] = data

  def ready(self):
    """ Check whether buffer has all the camera frames ready """
    if self.nb_cams == 0:
      raise RuntimeError("MulitCameraBuffer not initialized yet!")

    check_ts_same = (len(set(self._ts)) == 1)
    check_ts_len = (len(self._ts) == self.nb_cams)
    check_data = (len(self._data) == self.nb_cams)
    check_cam_indices = (len(set(self._data.keys())) == self.nb_cams)

    return check_ts_same and check_ts_len and check_data and check_cam_indices

  def get_camera_indices(self):
    """ Get camera indices """
    return self._data.keys()

  def get_data(self):
    """ Get camera data """
    if self.nb_cams is None:
      raise RuntimeError("MulitCameraBuffer not initialized yet!")

    return self._data


class BAFactor(Factor):
  """ BA Factor """
  def __init__(self, cam_geom, pids, z, covar=eye(2)):
    assert len(pids) == 3
    assert len(z) == 2
    assert covar.shape == (2, 2)
    Factor.__init__(self, "BAFactor", pids, z, covar)
    self.cam_geom = cam_geom

  def get_reproj_error(self, cam_pose, feature, cam_params):
    """ Get reprojection error """
    T_WC = pose2tf(cam_pose)
    p_W = feature
    p_C = tf_point(inv(T_WC), p_W)
    status, z_hat = self.cam_geom.project(cam_params, p_C)
    if status is False:
      return None

    z = self.measurement
    reproj_error = norm(z - z_hat)
    return reproj_error

  def eval(self, params, **kwargs):
    """ Evaluate """
    assert len(params) == 3
    assert len(params[0]) == 7
    assert len(params[1]) == 3
    assert len(params[2]) == self.cam_geom.get_params_size()

    # Setup
    r = np.array([0.0, 0.0])
    J0 = zeros((2, 6))
    J1 = zeros((2, 3))
    J2 = zeros((2, self.cam_geom.get_params_size()))

    # Map params
    cam_pose, feature, cam_params = params

    # Project point in world frame to image plane
    T_WC = pose2tf(cam_pose)
    z_hat = zeros((2, 1))
    p_W = zeros((3, 1))
    p_W = feature
    p_C = tf_point(inv(T_WC), p_W)
    status, z_hat = self.cam_geom.project(cam_params, p_C)

    # Calculate residual
    sqrt_info = self.sqrt_info
    z = self.measurement
    r = sqrt_info @ (z - z_hat)
    if kwargs.get('only_residuals', False):
      return r

    # Calculate Jacobians
    if status is False:
      return (r, [J0, J1, J2])
    # -- Measurement model jacobian
    neg_sqrt_info = -1.0 * sqrt_info
    Jh = self.cam_geom.J_proj(cam_params, p_C)
    Jh_weighted = neg_sqrt_info @ Jh
    # -- Jacobian w.r.t. camera pose T_WC
    C_WC = tf_rot(T_WC)
    C_CW = C_WC.T
    r_WC = tf_trans(T_WC)
    J0 = zeros((2, 6))  # w.r.t Camera pose T_WC
    J0[0:2, 0:3] = Jh_weighted @ -C_CW
    J0[0:2, 3:6] = Jh_weighted @ -C_CW @ skew(p_W - r_WC) @ -C_WC
    # -- Jacobian w.r.t. feature
    J1 = zeros((2, 3))
    J1 = Jh_weighted @ C_CW
    # -- Jacobian w.r.t. camera parameters
    J_cam_params = self.cam_geom.J_params(cam_params, p_C)
    J2 = zeros((2, self.cam_geom.get_params_size()))
    J2 = neg_sqrt_info @ J_cam_params

    return (r, [J0, J1, J2])


class VisionFactor(Factor):
  """ Vision Factor """
  def __init__(self, cam_geom, pids, z, covar=eye(2)):
    assert len(pids) == 4
    assert len(z) == 2
    assert covar.shape == (2, 2)
    Factor.__init__(self, "VisionFactor", pids, z, covar)
    self.cam_geom = cam_geom

  def get_reproj_error(self, pose, cam_exts, feature, cam_params):
    """ Get reprojection error """
    T_WB = pose2tf(pose)
    T_BCi = pose2tf(cam_exts)
    p_W = feature
    p_C = tf_point(inv(T_WB @ T_BCi), p_W)
    status, z_hat = self.cam_geom.project(cam_params, p_C)
    if status is False:
      return None

    z = self.measurement
    reproj_error = norm(z - z_hat)
    return reproj_error

  def eval(self, params, **kwargs):
    """ Evaluate """
    assert len(params) == 4
    assert len(params[0]) == 7
    assert len(params[1]) == 7
    assert len(params[2]) == 3
    assert len(params[3]) == self.cam_geom.get_params_size()

    # Setup
    r = np.array([0.0, 0.0])
    J0 = zeros((2, 6))
    J1 = zeros((2, 6))
    J2 = zeros((2, 3))
    J3 = zeros((2, self.cam_geom.get_params_size()))

    # Project point in world frame to image plane
    pose, cam_exts, feature, cam_params = params
    T_WB = pose2tf(pose)
    T_BCi = pose2tf(cam_exts)
    p_W = feature
    p_C = tf_point(inv(T_WB @ T_BCi), p_W)
    status, z_hat = self.cam_geom.project(cam_params, p_C)

    # Calculate residual
    sqrt_info = self.sqrt_info
    z = self.measurement
    r = sqrt_info @ (z - z_hat)
    if kwargs.get('only_residuals', False):
      return r

    # Calculate Jacobians
    if status is False:
      return (r, [J0, J1, J2, J3])

    C_BCi = tf_rot(T_BCi)
    C_WB = tf_rot(T_WB)
    C_CB = C_BCi.T
    C_BW = C_WB.T
    C_CW = C_CB @ C_WB.T
    r_WB = tf_trans(T_WB)
    neg_sqrt_info = -1.0 * sqrt_info
    # -- Measurement model jacobian
    Jh = self.cam_geom.J_proj(cam_params, p_C)
    Jh_weighted = neg_sqrt_info @ Jh
    # -- Jacobian w.r.t. pose T_WB
    J0 = zeros((2, 6))
    J0[0:2, 0:3] = Jh_weighted @ C_CB @ -C_BW
    J0[0:2, 3:6] = Jh_weighted @ C_CB @ -C_BW @ skew(p_W - r_WB) @ -C_WB
    # -- Jacobian w.r.t. camera extrinsics T_BCi
    J1 = zeros((2, 6))
    J1[0:2, 0:3] = Jh_weighted @ -C_CB
    J1[0:2, 3:6] = Jh_weighted @ -C_CB @ skew(C_BCi @ p_C) @ -C_BCi
    # -- Jacobian w.r.t. feature
    J2 = zeros((2, 3))
    J2 = Jh_weighted @ C_CW
    # -- Jacobian w.r.t. camera parameters
    J_cam_params = self.cam_geom.J_params(cam_params, p_C)
    J3 = zeros((2, 8))
    J3 = neg_sqrt_info @ J_cam_params

    return (r, [J0, J1, J2, J3])


class CalibVisionFactor(Factor):
  """ Calibration Vision Factor """
  def __init__(self, cam_geom, pids, grid_data, covar=eye(2)):
    assert len(pids) == 3
    assert len(grid_data) == 4
    assert covar.shape == (2, 2)
    tag_id, corner_idx, r_FFi, z = grid_data
    Factor.__init__(self, "CalibVisionFactor", pids, z, covar)
    self.cam_geom = cam_geom
    self.tag_id = tag_id
    self.corner_idx = corner_idx
    self.r_FFi = r_FFi

  def get_residual(self, pose, cam_exts, cam_params):
    """ Get residual """
    T_BF = pose2tf(pose)
    T_BCi = pose2tf(cam_exts)
    T_CiB = inv(T_BCi)
    r_CiFi = tf_point(T_CiB @ T_BF, self.r_FFi)
    status, z_hat = self.cam_geom.project(cam_params, r_CiFi)
    if status is False:
      return None

    r = self.measurement - z_hat
    return r

  def get_reproj_error(self, pose, cam_exts, cam_params):
    """ Get reprojection error """
    r = self.get_residual(pose, cam_exts, cam_params)
    if r is None:
      return None
    return norm(r)

  def eval(self, params, **kwargs):
    """ Evaluate """
    assert len(params) == 3
    assert len(params[0]) == 7
    assert len(params[1]) == 7
    assert len(params[2]) == self.cam_geom.get_params_size()

    # Setup
    r = np.array([0.0, 0.0])
    J0 = zeros((2, 6))
    J1 = zeros((2, 6))
    J2 = zeros((2, self.cam_geom.get_params_size()))

    # Map parameters out
    pose, cam_exts, cam_params = params
    T_BF = pose2tf(pose)
    T_BCi = pose2tf(cam_exts)

    # Transform and project point to image plane
    T_CiB = inv(T_BCi)
    r_CiFi = tf_point(T_CiB @ T_BF, self.r_FFi)
    status, z_hat = self.cam_geom.project(cam_params, r_CiFi)

    # Calculate residual
    sqrt_info = self.sqrt_info
    z = self.measurement
    r = sqrt_info @ (z - z_hat)
    if kwargs.get('only_residuals', False):
      return r

    # Calculate Jacobians
    if status is False:
      return (r, [J0, J1, J2])

    neg_sqrt_info = -1.0 * sqrt_info
    Jh = self.cam_geom.J_proj(cam_params, r_CiFi)
    Jh_weighted = neg_sqrt_info @ Jh
    # -- Jacobians w.r.t relative camera pose T_BF
    C_CiB = tf_rot(T_CiB)
    C_BF = tf_rot(T_BF)
    J0 = zeros((2, 6))
    J0[0:2, 0:3] = Jh_weighted @ C_CiB
    J0[0:2, 3:6] = Jh_weighted @ C_CiB @ -C_BF @ skew(self.r_FFi)
    # -- Jacobians w.r.t T_BCi
    r_BFi = tf_point(T_BF, self.r_FFi)
    r_BCi = tf_trans(T_BCi)
    C_BCi = tf_rot(T_BCi)
    J1 = zeros((2, 6))
    J1[0:2, 0:3] = Jh_weighted @ -C_CiB
    J1[0:2, 3:6] = Jh_weighted @ -C_CiB @ skew(r_BFi - r_BCi) @ -C_BCi
    # -- Jacobians w.r.t cam params
    J_cam_params = self.cam_geom.J_params(cam_params, r_CiFi)
    J2 = neg_sqrt_info @ J_cam_params

    return (r, [J0, J1, J2])


class ImuBuffer:
  """ IMU buffer """
  def __init__(self, ts=None, acc=None, gyr=None):
    self.ts = ts if ts is not None else []
    self.acc = acc if acc is not None else []
    self.gyr = gyr if gyr is not None else []

  def add(self, ts, acc, gyr):
    """ Add imu measurement """
    self.ts.append(ts)
    self.acc.append(acc)
    self.gyr.append(gyr)

  def add_event(self, imu_event):
    """ Add imu event """
    self.ts.append(imu_event.ts)
    self.acc.append(imu_event.acc)
    self.gyr.append(imu_event.gyr)

  def length(self):
    """ Return length of imu buffer """
    return len(self.ts)

  def extract(self, ts_start, ts_end):
    """ Form ImuBuffer """
    assert ts_start >= self.ts[0]
    assert ts_end <= self.ts[-1]
    imu_ts = []
    imu_acc = []
    imu_gyr = []

    # Extract data between ts_start and ts_end
    remove_idx = 0
    ts_km1 = None
    acc_km1 = None
    gyr_km1 = None

    for k, ts_k in enumerate(self.ts):
      # Check if within the extraction zone
      if ts_k < ts_start:
        continue

      # Setup
      acc_k = self.acc[k]
      gyr_k = self.gyr[k]

      # Interpolate start or end?
      if len(imu_ts) == 0 and ts_k > ts_start:
        # Interpolate start
        ts_km1 = self.ts[k - 1]
        acc_km1 = self.acc[k - 1]
        gyr_km1 = self.gyr[k - 1]

        alpha = (ts_start - ts_km1) / (ts_k - ts_km1)
        acc_km1 = (1.0 - alpha) * acc_km1 + alpha * acc_k
        gyr_km1 = (1.0 - alpha) * gyr_km1 + alpha * gyr_k
        ts_km1 = ts_start

        imu_ts.append(ts_km1)
        imu_acc.append(acc_km1)
        imu_gyr.append(gyr_km1)

      elif ts_k > ts_end:
        # Interpolate end
        ts_km1 = self.ts[k - 1]
        acc_km1 = self.acc[k - 1]
        gyr_km1 = self.gyr[k - 1]

        alpha = (ts_end - ts_km1) / (ts_k - ts_km1)
        acc_k = (1.0 - alpha) * acc_km1 + alpha * acc_k
        gyr_k = (1.0 - alpha) * gyr_km1 + alpha * gyr_k
        ts_k = ts_end

      # Add to subset
      imu_ts.append(ts_k)
      imu_acc.append(acc_k)
      imu_gyr.append(gyr_k)

      # End?
      if ts_k == ts_end:
        break

      # Update
      remove_idx = k

    # Remove data before ts_end
    self.ts = self.ts[remove_idx:]
    self.acc = self.acc[remove_idx:]
    self.gyr = self.gyr[remove_idx:]

    return ImuBuffer(imu_ts, imu_acc, imu_gyr)

  def print(self, extra_newline=False):
    """ Print """
    for ts, acc, gyr in zip(self.ts, self.acc, self.gyr):
      print(f"ts: [{ts}], acc: {acc}, gyr: {gyr}")

    if extra_newline:
      print()


@dataclass
class ImuParams:
  """ IMU parameters """
  noise_acc: np.array
  noise_gyr: np.array
  noise_ba: np.array
  noise_bg: np.array
  g: np.array = np.array([0.0, 0.0, 9.81])


@dataclass
class ImuFactorData:
  """ IMU Factor data """
  state_F: np.array
  state_P: np.array
  dr: np.array
  dv: np.array
  dC: np.array
  ba: np.array
  bg: np.array
  g: np.array
  Dt: float


class ImuFactor(Factor):
  """ Imu Factor """
  def __init__(self, pids, imu_params, imu_buf, sb_i):
    assert len(pids) == 4
    self.imu_params = imu_params
    self.imu_buf = imu_buf

    data = self.propagate(imu_buf, imu_params, sb_i)
    Factor.__init__(self, "ImuFactor", pids, None, data.state_P)

    self.state_F = data.state_F
    self.state_P = data.state_P
    self.dr = data.dr
    self.dv = data.dv
    self.dC = data.dC
    self.ba = data.ba
    self.bg = data.bg
    self.g = data.g
    self.Dt = data.Dt

  @staticmethod
  def propagate(imu_buf, imu_params, sb_i):
    """ Propagate imu measurements """
    # Setup
    Dt = 0.0
    g = imu_params.g
    state_F = eye(15)  # State jacobian
    state_P = zeros((15, 15))  # State covariance

    # Noise matrix Q
    Q = zeros((12, 12))
    Q[0:3, 0:3] = imu_params.noise_acc**2 * eye(3)
    Q[3:6, 3:6] = imu_params.noise_gyr**2 * eye(3)
    Q[6:9, 6:9] = imu_params.noise_ba**2 * eye(3)
    Q[9:12, 9:12] = imu_params.noise_bg**2 * eye(3)

    # Pre-integrate relative position, velocity, rotation and biases
    dr = np.array([0.0, 0.0, 0.0])  # Relative position
    dv = np.array([0.0, 0.0, 0.0])  # Relative velocity
    dC = eye(3)  # Relative rotation
    ba_i = sb_i.param[3:6]  # Accel biase at i
    bg_i = sb_i.param[6:9]  # Gyro biase at i

    # Pre-integrate imu measuremenets
    for k in range(len(imu_buf.ts) - 1):
      # Timestep
      ts_i = imu_buf.ts[k]
      ts_j = imu_buf.ts[k + 1]
      dt = ts2sec(ts_j - ts_i)
      dt_sq = dt * dt

      # Accelerometer and gyroscope measurements
      acc_i = imu_buf.acc[k]
      gyr_i = imu_buf.gyr[k]

      # Propagate IMU state using Euler method
      dr = dr + (dv * dt) + (0.5 * dC @ (acc_i - ba_i) * dt_sq)
      dv = dv + dC @ (acc_i - ba_i) * dt
      dC = dC @ Exp((gyr_i - bg_i) * dt)
      ba = ba_i
      bg = bg_i

      # Make sure determinant of rotation is 1 by normalizing the quaternion
      dq = quat_normalize(rot2quat(dC))
      dC = quat2rot(dq)

      # Continuous time transition matrix F
      F = zeros((15, 15))
      F[0:3, 3:6] = eye(3)
      F[3:6, 6:9] = -1.0 * dC @ skew(acc_i - ba_i)
      F[3:6, 9:12] = -1.0 * dC
      F[6:9, 6:9] = -1.0 * skew(gyr_i - bg_i)
      F[6:9, 12:15] = -eye(3)

      # Continuous time input jacobian G
      G = zeros((15, 12))
      G[3:6, 0:3] = -1.0 * dC
      G[6:9, 3:6] = -eye(3)
      G[9:12, 6:9] = eye(3)
      G[12:15, 9:12] = eye(3)

      # Update
      G_dt = G * dt
      I_F_dt = eye(15) + F * dt
      state_F = I_F_dt @ state_F
      state_P = I_F_dt @ state_P @ I_F_dt.T + G_dt @ Q @ G_dt.T
      Dt += dt

    state_P = (state_P + state_P.T) / 2.0
    return ImuFactorData(state_F, state_P, dr, dv, dC, ba, bg, g, Dt)

  def eval(self, params, **kwargs):
    """ Evaluate IMU factor """
    assert len(params) == 4
    assert len(params[0]) == 7
    assert len(params[1]) == 9
    assert len(params[2]) == 7
    assert len(params[3]) == 9

    # Map params
    pose_i, sb_i, pose_j, sb_j = params

    # Timestep i
    T_i = pose2tf(pose_i)
    r_i = tf_trans(T_i)
    C_i = tf_rot(T_i)
    q_i = tf_quat(T_i)
    v_i = sb_i[0:3]
    ba_i = sb_i[3:6]
    bg_i = sb_i[6:9]

    # Timestep j
    T_j = pose2tf(pose_j)
    r_j = tf_trans(T_j)
    C_j = tf_rot(T_j)
    q_j = tf_quat(T_j)
    v_j = sb_j[0:3]

    # Correct the relative position, velocity and orientation
    # -- Extract jacobians from error-state jacobian
    dr_dba = self.state_F[0:3, 9:12]
    dr_dbg = self.state_F[0:3, 12:15]
    dv_dba = self.state_F[3:6, 9:12]
    dv_dbg = self.state_F[3:6, 12:15]
    dq_dbg = self.state_F[6:9, 12:15]
    dba = ba_i - self.ba
    dbg = bg_i - self.bg
    # -- Correct the relative position, velocity and rotation
    dr = self.dr + dr_dba @ dba + dr_dbg @ dbg
    dv = self.dv + dv_dba @ dba + dv_dbg @ dbg
    dC = self.dC @ Exp(dq_dbg @ dbg)
    dq = quat_normalize(rot2quat(dC))

    # Form residuals
    sqrt_info = self.sqrt_info
    g = self.g
    Dt = self.Dt
    Dt_sq = Dt * Dt

    dr_meas = (C_i.T @ ((r_j - r_i) - (v_i * Dt) + (0.5 * g * Dt_sq)))
    dv_meas = (C_i.T @ ((v_j - v_i) + (g * Dt)))

    err_pos = dr_meas - dr
    err_vel = dv_meas - dv
    err_rot = (2.0 * quat_mul(quat_inv(dq), quat_mul(quat_inv(q_i), q_j)))[1:4]
    err_ba = np.array([0.0, 0.0, 0.0])
    err_bg = np.array([0.0, 0.0, 0.0])
    r = sqrt_info @ np.block([err_pos, err_vel, err_rot, err_ba, err_bg])
    if kwargs.get('only_residuals', False):
      return r

    # Form jacobians
    J0 = zeros((15, 6))  # residuals w.r.t pose i
    J1 = zeros((15, 9))  # residuals w.r.t speed and biase i
    J2 = zeros((15, 6))  # residuals w.r.t pose j
    J3 = zeros((15, 9))  # residuals w.r.t speed and biase j

    # -- Jacobian w.r.t. pose i
    # yapf: disable
    J0[0:3, 0:3] = -C_i.T  # dr w.r.t r_i
    J0[0:3, 3:6] = skew(dr_meas)  # dr w.r.t C_i
    J0[3:6, 3:6] = skew(dv_meas)  # dv w.r.t C_i
    J0[6:9, 3:6] = -(quat_left(rot2quat(C_j.T @ C_i)) @ quat_right(dq))[1:4, 1:4]  # dtheta w.r.t C_i
    J0 = sqrt_info @ J0
    # yapf: enable

    # -- Jacobian w.r.t. speed and biases i
    # yapf: disable
    J1[0:3, 0:3] = -C_i.T * Dt  # dr w.r.t v_i
    J1[0:3, 3:6] = -dr_dba  # dr w.r.t ba
    J1[0:3, 6:9] = -dr_dbg  # dr w.r.t bg
    J1[3:6, 0:3] = -C_i.T  # dv w.r.t v_i
    J1[3:6, 3:6] = -dv_dba  # dv w.r.t ba
    J1[3:6, 6:9] = -dv_dbg  # dv w.r.t bg
    J1[6:9, 6:9] = -quat_left(rot2quat(C_j.T @ C_i @ self.dC))[1:4, 1:4] @ dq_dbg  # dtheta w.r.t C_i
    J1 = sqrt_info @ J1
    # yapf: enable

    # -- Jacobian w.r.t. pose j
    # yapf: disable
    J2[0:3, 0:3] = C_i.T  # dr w.r.t r_j
    J2[6:9, 3:6] = quat_left(rot2quat(dC.T @ C_i.T @ C_j))[1:4, 1:4]  # dtheta w.r.t C_j
    J2 = sqrt_info @ J2
    # yapf: enable

    # -- Jacobian w.r.t. sb j
    J3[3:6, 0:3] = C_i.T  # dv w.r.t v_j
    J3 = sqrt_info @ J3

    return (r, [J0, J1, J2, J3])


class MargFactor(Factor):
  """ Marginalization Factor """

  def __init__(self, pids):
    assert len(pids) > 0
    Factor.__init__(self, "MargFactor", pids, None, data.state_P)

    self.r0 = None  # Linearized residuals
    self.J0 = None  # Linearized jacobians
    self.x0 = None  # Linearized estimates

  def shur_complement(H, b, m, r):
    """ Schur complement """
    H_mm = H[0:m, 0:m]
    H_mr = H[0:m, m:]
    H_rm = H[m:, 0:m]
    H_rr = H[m:, m:]

    b_mm = b[:m]
    b_rr = b[m:]

    H_mm = 0.5 * (H_mm + H_mm.T)  # Enforce symmetry
    w, V = np.linalg.eig(H_mm)
    for idx, w_i in enumerate(w):
      if w_i < 1e-12:
        w[idx] = 0.0

    Lambda_inv = np.diag(1.0 / w)
    H_mm_inv = V @ Lambda_inv @ V.T
    H_marg = H_rr - (H_rm @ H_mm_inv @ H_mr)
    b_marg = b_rr - (H_rm @ H_mm_inv @ b_mm)

    return (H_marg, b_marg)

  def decomp_hessian(H):
    """ Decompose Hessian into E' * E """
    w, V = np.linalg.eig(H)
    for idx, w_i in enumerate(w):
      if w_i < 1e-12:
        w[idx] = 0.0

    S_sqrt = np.diag(w**0.5)
    E = S_sqrt @ V.T

    return E

  def marginalize(self):
    """ Marginalize """
    pass

  def calc_delta_chi(self):
    """ Calculate Delta Chi """
    pass

  def eval(self, params, **kwargs):
    """ Evaluate Marginalization Factor """

    r = np.zeros((2, 2))
    if kwargs.get('only_residuals', False):
      return r



# SOLVER #######################################################################


class Solver:
  """ Solver """
  def __init__(self):
    self.factors = {}
    self.params = {}
    self.solver_max_iter = 5
    self.solver_lambda = 1e4

  def add(self, factor, factor_params):
    """ Add factor and parameters """
    assert factor.factor_id is not None
    assert len(factor_params) >= 1
    assert factor_params[0].param_id is not None
    self.factors[factor.factor_id] = factor
    for param in factor_params:
      self.params[param.param_id] = param

  def add_param(self, param):
    """ Add param """
    self.params[param.param_id] = param

  def add_factor(self, factor):
    """ Add factor """
    # Double check if params exists
    for param_id in factor.param_ids:
      if param_id not in self.params:
        raise RuntimeError(f"Parameter [{param_id}] does not exist!")

    # Add factor
    self.factors[factor.factor_id] = factor

  @staticmethod
  def _print_to_console(iter_k, lambda_k, cost_kp1, cost_k):
    """ Print to console """

    print(f"iter[{iter_k}]:", end=" ")
    print(f"lambda: {lambda_k:.2e}", end=", ")
    print(f"cost: {cost_kp1:.2e}", end=", ")
    print(f"dcost: {cost_kp1 - cost_k:.2e}", end=" ")
    print()

    # rmse_vision = rmse(self._get_reproj_errors())
    # print(f"rms_reproj_error: {rmse_vision:.2f} px")

    sys.stdout.flush()

  def _form_param_indices(self):
    """ Form parameter indices """
    # Parameter ids
    param_ids = {
        'pose': set(),
        'speed_and_biases': set(),
        'feature': set(),
        'camera': set(),
        'extrinsics': set(),
    }

    # Track parameters
    nb_params = 0
    for _, factor in self.factors.items():
      for _, param_id in enumerate(factor.param_ids):
        param = self.params[param_id]
        if param.fix:
          continue
        else:
          param_ids[param.var_type].add(param_id)
        nb_params += 1

    # Assign global parameter order
    param_order = []
    param_order.append("pose")
    param_order.append("speed_and_biases")
    param_order.append("feature")
    param_order.append("camera")
    param_order.append("extrinsics")

    param_idxs = {}
    param_size = 0
    for param_type in param_order:
      for param_id in param_ids[param_type]:
        param_idxs[param_id] = param_size
        param_size += self.params[param_id].min_dims

    return (param_idxs, param_size)

  def _linearize(self, params):
    """ Linearize non-linear problem """
    # Setup
    (param_idxs, param_size) = self._form_param_indices()
    H = zeros((param_size, param_size))
    g = zeros(param_size)

    # Form Hessian and R.H.S of Gauss newton
    for _, factor in self.factors.items():
      factor_params = [params[pid].param for pid in factor.param_ids]
      r, jacobians = factor.eval(factor_params)

      # Form Hessian
      nb_params = len(factor_params)
      for i in range(nb_params):
        param_i = params[factor.param_ids[i]]
        if param_i.fix:
          continue
        idx_i = param_idxs[factor.param_ids[i]]
        size_i = param_i.min_dims
        J_i = jacobians[i]

        for j in range(i, nb_params):
          param_j = params[factor.param_ids[j]]
          if param_j.fix:
            continue
          idx_j = param_idxs[factor.param_ids[j]]
          size_j = param_j.min_dims
          J_j = jacobians[j]

          rs = idx_i
          re = idx_i + size_i
          cs = idx_j
          ce = idx_j + size_j

          if i == j:  # Diagonal
            H[rs:re, cs:ce] += J_i.T @ J_j
          else:  # Off-Diagonal
            H[rs:re, cs:ce] += J_i.T @ J_j
            H[cs:ce, rs:re] += H[rs:re, cs:ce].T

        # Form R.H.S. Gauss Newton g
        rs = idx_i
        re = idx_i + size_i
        g[rs:re] += (-J_i.T @ r)

    return (H, g, param_idxs)

  def _calculate_residuals(self, params):
    """ Calculate Residuals """
    residuals = np.array([])

    for _, factor in self.factors.items():
      factor_params = [params[pid].param for pid in factor.param_ids]
      r = factor.eval(factor_params, only_residuals=True)
      # residuals.append(r)
      residuals = np.append(residuals, r)

    # return np.array(residuals).flatten()
    return residuals

  def _calculate_cost(self, params):
    """ Calculate Cost """
    r = self._calculate_residuals(params)
    return 0.5 * (r.T @ r)

  @staticmethod
  def _update(params_k, param_idxs, dx):
    """ Update """
    params_kp1 = copy.deepcopy(params_k)

    for param_id, param in params_kp1.items():
      # Check if param even exists
      if param_id not in param_idxs:
        continue

      # Update parameter
      start = param_idxs[param_id]
      end = start + param.min_dims
      param_dx = dx[start:end]
      update_state_variable(param, param_dx)

    return params_kp1

  @staticmethod
  def _solve_for_dx(lambda_k, H, g):
    """ Solve for dx """
    # Damp Hessian
    H = H + lambda_k * eye(H.shape[0])
    # H = H + lambda_k * np.diag(H.diagonal())

    # # Pseudo inverse
    # dx = pinv(H) @ g

    # Cholesky decomposition
    c, low = scipy.linalg.cho_factor(H)
    dx = scipy.linalg.cho_solve((c, low), g)

    # SVD
    # dx = solve_svd(H, g)

    # # QR
    # q, r = np.linalg.qr(H)
    # p = np.dot(q.T, g)
    # dx = np.dot(np.linalg.inv(r), p)

    # # Sparse cholesky decomposition
    # sH = scipy.sparse.csc_matrix(H)
    # dx = scipy.sparse.linalg.spsolve(sH, g)
    # dx = scipy.sparse.linalg.spsolve(H, g, permc_spec="NATURAL")

    return dx

  def solve(self, verbose=False):
    """ Solve """
    lambda_k = self.solver_lambda
    params_k = copy.deepcopy(self.params)
    cost_k = self._calculate_cost(params_k)

    # First evaluation
    if verbose:
      print(f"nb_factors: {len(self.factors)}")
      print(f"nb_params: {len(self.params)}")
      self._print_to_console(0, lambda_k, cost_k, cost_k)

    # Iterate
    for i in range(1, self.solver_max_iter):
      # Update and calculate cost
      (H, g, param_idxs) = self._linearize(params_k)
      dx = self._solve_for_dx(lambda_k, H, g)
      params_kp1 = self._update(params_k, param_idxs, dx)
      cost_kp1 = self._calculate_cost(params_kp1)

      # Verbose
      if verbose:
        self._print_to_console(i, lambda_k, cost_kp1, cost_k)

      # Accept or reject update
      # dcost = cost_kp1 - cost_k
      if cost_kp1 < cost_k:
        # Accept update
        cost_k = cost_kp1
        params_k = params_kp1
        lambda_k /= 10.0

      else:
        # Reject update
        params_k = params_k
        lambda_k *= 10.0

      # # Termination criteria
      # if dcost > -1e-5:
      #   break

    # (H, _, _) = self._linearize(params_k)
    # P = np.linalg.inv(H)
    # P = P[-8:, -8:]
    # n = P.shape[0]
    # print(f"n: {n}")
    # print(f"det(P): {np.linalg.det(P)}", end=", ")
    # print(f"rank(P): {np.linalg.matrix_rank(P)}", end=", ")
    # print(f"size(P): {P.shape}", end=", ")
    # print(f"H(P): {0.5 * np.log((2.0 * pi * np.exp(1))**n * np.linalg.det(P))}")

    # Finish - set the original params the optimized values
    # Note: The reason we don't just do `self.params = params_k` is because
    # that would destroy the references to outside `FactorGraph()`.
    for param_id, param in params_k.items():
      self.params[param_id].param = param.param


# FACTOR GRAPH ################################################################


class FactorGraph:
  """ Factor Graph """
  def __init__(self):
    # Parameters and factors
    self._next_param_id = 0
    self._next_factor_id = 0
    self.params = {}
    self.factors = {}
    self.solver_max_iter = 5
    self.solver_lambda = 1e4

  def add_param(self, param):
    """ Add param """
    param_id = self._next_param_id
    self.params[param_id] = param
    self.params[param_id].set_param_id(param_id)
    self._next_param_id += 1
    return param_id

  def add_factor(self, factor):
    """ Add factor """
    # Double check if params exists
    for param_id in factor.param_ids:
      if param_id not in self.params:
        raise RuntimeError(f"Parameter [{param_id}] does not exist!")

    # Add factor
    factor_id = self._next_factor_id
    self.factors[factor_id] = factor
    self.factors[factor_id].set_factor_id(factor_id)
    self._next_factor_id += 1
    return factor_id

  def remove_param(self, param):
    """ Remove param """
    assert param.param_id in self.params
    del self.params[param.param_id]

  def remove_factor(self, factor):
    """ Remove factor """
    assert factor.factor_id in self.factors
    del self.factors[factor.factor_id]

  def get_reproj_errors(self):
    """ Get reprojection errors """
    target_factors = ["BAFactor", "VisionFactor", "CalibVisionFactor"]

    reproj_errors = []
    for _, factor in self.factors.items():
      if factor.factor_type in target_factors:
        factor_params = [self.params[pid].param for pid in factor.param_ids]
        retval = factor.get_reproj_error(*factor_params)
        if retval is not None:
          reproj_errors.append(retval)

    return np.array(reproj_errors).flatten()

  def solve(self, verbose=False):
    """ Solve """
    solver = Solver()
    solver.solver_max_iter = self.solver_max_iter
    solver.solver_lambda = self.solver_lambda
    for _, factor in self.factors.items():
      factor_params = [self.params[pid] for pid in factor.param_ids]
      solver.add(factor, factor_params)
    solver.solve(verbose)


# KALMAN FILTER ################################################################


class KalmanFilter:
  """
  Kalman Filter

  Predict
  x = F * x + B * u;
  P = F * P * F' + G * Q * G';

  Update
  y = z - H * x
  K = P * T' * inv(T * P * T' + R);
  x = x + K * y;
  P = (I - K * T) * P * (I - K * T)' + K * R * K';
  """
  def __init__(self):
    pass


# MSCKF ########################################################################


class MSCKF:
  """ Multi-State Constraint Kalman Filter """
  def __init__(self):
    # Settings
    self.window_size = 5
    # -- IMU
    self.imu_rate = 200.0
    # -- Process noise
    self.sigma_na = 0.01
    self.sigma_nw = 0.01
    self.sigma_ba = 0.01
    self.sigma_bg = 0.01

    # Stats
    self.prev_ts = 0.0

    # State Vector [r, v, q, ba, bg]
    self.state_r_WS = np.array([0.0, 0.0, 0.0])
    self.state_v_WS = np.array([0.0, 0.0, 0.0])
    self.state_C_WS = eye(3)
    self.state_ba = np.array([0.0, 0.0, 0.0])
    self.state_bg = np.array([0.0, 0.0, 0.0])
    self.state_g = np.array([0, 0, 9.81])

    # State Jacobian and covariance
    self.F = eye(15)
    self.P = eye(15)

    # Process noise matrix Q
    self.Q = zeros((12, 12))
    self.Q[0:3, 0:3] = self.sigma_na**2 * eye(3)
    self.Q[3:6, 3:6] = self.sigma_nw**2 * eye(3)
    self.Q[6:9, 6:9] = self.sigma_ba**2 * eye(3)
    self.Q[9:12, 9:12] = self.sigma_bg**2 * eye(3)

    # Features
    self.features = {}

  def prediction_update(self, ts, a_m, w_m):
    """ Prediction update """
    # Setup
    v_WS_k = self.state_v_WS
    C_WS_k = self.state_C_WS
    q_WS_k = rot2quat(C_WS_k)
    ba = self.state_ba
    bg = self.state_bg
    g = self.state_g
    dt = 1.0 / self.imu_rate

    # Prediction update via Runge-Kutta 4th Order
    a = a_m - ba
    w = w_m - bg
    # -- Integrate orientation at time k + dt (kpdt: k plus dt)
    q_WS_kpdt = quat_integrate(q_WS_k, w, dt)
    C_WS_kpdt = quat2rot(q_WS_kpdt)
    # -- Integrate orientation at time k + dt / 2 (kphdt: k plus half dt)
    q_WS_kphdt = quat_integrate(q_WS_k, w, dt / 2.0)
    C_WS_kphdt = quat2rot(q_WS_kphdt)
    # -- k1 = f(tn, yn)
    k1_v_dot = C_WS_k @ a - g
    k1_p_dot = v_WS_k
    # -- k2 = f(tn + dt / 2, yn + k1 * dt / 2)
    k2_v_dot = C_WS_kphdt @ a - g
    k2_p_dot = v_WS_k + k1_v_dot * dt / 2.0
    # -- k3 = f(tn + dt / 2, yn + k2 * dt / 2)
    k3_v_dot = C_WS_kphdt @ a - g
    k3_p_dot = v_WS_k + k2_v_dot * dt / 2.0
    # -- k4 = f(tn + dt, tn + k3 * dt)
    k4_v_dot = C_WS_kpdt @ a - g
    k4_p_dot = v_WS_k + k3_v_dot * dt
    # -- Update predicted state
    self.state_r_WS += dt / 6.0 * (k1_p_dot + 2.0 * k2_p_dot + 2.0 * k3_p_dot +
                                   k4_p_dot)
    self.state_v_WS += dt / 6.0 * (k1_v_dot + 2.0 * k2_v_dot + 2.0 * k3_v_dot +
                                   k4_v_dot)
    self.state_C_WS = C_WS_kpdt

    # Jacobian of process model w.r.t. error vector - F
    F = zeros((15, 15))
    F[0:3, 3:6] = eye(3)
    F[3:6, 6:9] = -1.0 * self.state_C_WS @ skew(a)
    F[3:6, 9:12] = -1.0 * self.state_C_WS
    F[6:9, 6:9] = -1.0 * skew(w)
    F[6:9, 13:15] = -eye(3)

    # Jacobian of process model w.r.t. impulse vector - G
    G = zeros((15, 12))
    G[3:6, 0:3] = -1.0 * self.state_C_WS
    G[6:9, 3:6] = -eye(3)
    G[9:12, 6:9] = eye(3)
    G[13:15, 9:12] = eye(3)

    # State covariance matrix P
    G_dt = G * dt
    I_F_dt = eye(15) + F * dt
    self.F = I_F_dt @ self.F
    self.P = I_F_dt @ self.P @ I_F_dt.T + G_dt @ self.Q @ G_dt.T
    self.prev_ts = ts

  # def augment_state(self):
  #   T_WS = tf(self.C_WS, self.r_WS)
  #   T_SC = tf(self.C_WS, self.r_WS)
  #
  #   r_SC = tf_trans(T_SC)
  #   C_SC = tf_rot(T_SC)
  #   C_WS = tf_rot(T_WS)
  #
  #   J = zeros(6, 15 + self.window_size)
  #   J[0:3, 0:3] = C_SC
  #   J[3:6, 0:3] = skew(C_WS @ r_SC)
  #
  #   P_aug = np.block([eye(6 * self.window_size + 15), J])
  #   self.P = P_aug @ self.P @ P_aug.T
  #
  # def feature_jacobian(self, ts, feature_ids, keypoints):
  #   # Get camera pose in world frame
  #   T_WS = tf(self.C_WS, self.r_WS)
  #   T_SC = tf(self.C_SC, self.r_SC)
  #   T_WC = T_WS @ T_SC
  #
  #   # Transform feature position to image point z_hat
  #   p_W = self.features[feature_id]
  #   p_C = tf_point(inv(T_WC), p_W)
  #   C_WC = tf_rot(T_WC)
  #   z_hat = np.array([p_C(1) / p_C(3), p_C(2) / p_C(3)])
  #
  #   # Form residual
  #   r = z - z_hat
  #
  #   # Form jacobians
  #   J_i = 1.0 / p_C(3) * np.array(
  #       [1.0, 0.0, -p_C(1) / p_C(3), 0.0, 1.0, -p_C(2) / p_C(3)])
  #   H_x = [J_i * skew(p_C), -J_i * skew(C_WC)]
  #   H_f = J_i * skew(C_WC)
  #
  #   # Project residuals and feature Jacobian to Null Space of state Jacobian
  #   [U, S, V] = svd(H_f)
  #   A = U[:, 0:(2 * nb_cam_states)]
  #   r = A.T * H_f
  #   H_x = A.T * H_x
  #
  #   return (r, H_x)
  #
  # def measurement_jacobian(msckf, ts, feature_ids, keypoints):
  #   # Iterate over all features currently tracking and form a measurement
  #   # jacobian
  #   H_x = []  # State jacobian
  #   H_f = []  # Feature jacobian
  #
  #   # for i, fid in enumerate(feature_ids):
  #   #   feature_id = feature_ids(i)
  #   #   z = keypoints{i}
  #   return (r, H_x)
  #
  # def measurement_update(self, ts, feature_ids, keypoints):
  #   self.augment_state()
  #
  #   # Marginalize out feature jacobians and residuals via QR decomposition
  #   H = []
  #   r = []
  #   [Q, R] = qr(H)
  #   H_thin = (Q.T @ H)[0:(15 + 6 * nb_cam_states), 1:end]
  #   r_thin = (Q.T @ r)[0:(15 + 6 * nb_cam_states)]
  #
  #   # Calculate update
  #   # K = P * T.T * inv(T * P * T.T + R)
  #   K = self.P @ H_thin.T @ inv(H_thin @ self.P @ H_thin.T + self.R)
  #   dx = K @ r_thin
  #
  #   # Update state-vector
  #
  #   # Update state-covariance
  #   I_KH = (eye() - K @ H_thin)
  #   self.P = I_KH @ self.P @ I_KH.T + K @ self.R @ K.T


# FEATURE TRACKING #############################################################


def draw_matches(img_i, img_j, kps_i, kps_j, **kwargs):
  """
  Draw keypoint matches between images `img_i` and `img_j` with keypoints
  `kps_i` and `kps_j`
  """
  assert len(kps_i) == len(kps_j)

  nb_kps = len(kps_i)
  viz = cv2.hconcat([img_i, img_j])
  viz = cv2.cvtColor(viz, cv2.COLOR_GRAY2RG)

  color = (0, 255, 0)
  radius = 3
  thickness = kwargs.get('thickness', cv2.FILLED)
  linetype = kwargs.get('linetype', cv2.LINE_AA)

  for n in range(nb_kps):
    pt_i = None
    pt_j = None
    if hasattr(kps_i[n], 'pt'):
      pt_i = (int(kps_i[n].pt[0]), int(kps_i[n].pt[1]))
      pt_j = (int(kps_j[n].pt[0] + img_i.shape[1]), int(kps_j[n].pt[1]))
    else:
      pt_i = (int(kps_i[n][0]), int(kps_i[n][1]))
      pt_j = (int(kps_j[n][0] + img_i.shape[1]), int(kps_j[n][1]))

    cv2.circle(viz, pt_i, radius, color, thickness, lineType=linetype)
    cv2.circle(viz, pt_j, radius, color, thickness, lineType=linetype)
    cv2.line(viz, pt_i, pt_j, color, 1, linetype)

  return viz


def draw_keypoints(img, kps, inliers=None, **kwargs):
  """
  Draw points `kps` on image `img`. The `inliers` boolean list is optional
  and is expected to be the same size as `kps` denoting whether the point
  should be drawn or not.
  """
  inliers = [1 for i in range(len(kps))] if inliers is None else inliers
  radius = kwargs.get('radius', 2)
  color = kwargs.get('color', (0, 255, 0))
  thickness = kwargs.get('thickness', cv2.FILLED)
  linetype = kwargs.get('linetype', cv2.LINE_AA)

  viz = img
  if len(img.shape) == 2:
    viz = cv2.cvtColor(img, cv2.COLOR_GRAY2RGB)

  for n, kp in enumerate(kps):
    if inliers[n]:
      p = None
      if hasattr(kp, 'pt'):
        p = (int(kp.pt[0]), int(kp.pt[1]))
      else:
        p = (int(kp[0]), int(kp[1]))

      cv2.circle(viz, p, radius, color, thickness, lineType=linetype)

  return viz


def sort_keypoints(kps):
  """ Sort a list of cv2.KeyPoint based on their response """
  responses = [kp.response for kp in kps]
  indices = range(len(responses))
  indices = sorted(indices, key=lambda i: responses[i], reverse=True)
  return [kps[i] for i in indices]


def spread_keypoints(img, kps, min_dist, **kwargs):
  """
  Given a set of keypoints `kps` make sure they are atleast `min_dist` pixels
  away from each other, if they are not remove them.
  """
  # Pre-check
  if not kps:
    return kps

  # Setup
  debug = kwargs.get('debug', False)
  prev_kps = kwargs.get('prev_kps', [])
  min_dist = int(min_dist)
  img_h, img_w = img.shape
  A = np.zeros(img.shape)  # Allowable areas are marked 0 else not allowed

  # Loop through previous keypoints
  for kp in prev_kps:
    # Convert from keypoint to tuple
    p = (int(kp.pt[0]), int(kp.pt[1]))

    # Fill the area of the matrix where the next keypoint cannot be around
    rs = int(max(p[1] - min_dist, 0.0))
    re = int(min(p[1] + min_dist + 1, img_h))
    cs = int(max(p[0] - min_dist, 0.0))
    ce = int(min(p[0] + min_dist + 1, img_w))
    A[rs:re, cs:ce] = np.ones((re - rs, ce - cs))

  # Loop through keypoints
  kps_results = []
  for kp in sort_keypoints(kps):
    # Convert from keypoint to tuple
    p = (int(kp.pt[0]), int(kp.pt[1]))

    # Check if point is ok to be added to results
    if A[p[1], p[0]] > 0.0:
      continue

    # Fill the area of the matrix where the next keypoint cannot be around
    rs = int(max(p[1] - min_dist, 0.0))
    re = int(min(p[1] + min_dist + 1, img_h))
    cs = int(max(p[0] - min_dist, 0.0))
    ce = int(min(p[0] + min_dist + 1, img_w))
    A[rs:re, cs:ce] = np.ones((re - rs, ce - cs))
    A[p[1], p[0]] = 2

    # Add to results
    kps_results.append(kp)

  # Debug
  if debug:
    img = draw_keypoints(img, kps_results, radius=3)

    plt.figure()

    ax = plt.subplot(121)
    ax.imshow(A)
    ax.set_xlabel('pixel')
    ax.set_ylabel('pixel')
    ax.xaxis.tick_top()
    ax.xaxis.set_label_position('top')

    ax = plt.subplot(122)
    ax.imshow(img)
    ax.set_xlabel('pixel')
    ax.set_ylabel('pixel')
    ax.xaxis.tick_top()
    ax.xaxis.set_label_position('top')

    plt.show()

  return kps_results


class FeatureGrid:
  """
  FeatureGrid

  The idea is to take all the feature positions and put them into grid cells
  across the full image space. This is so that one could keep track of how many
  feautures are being tracked in each individual grid cell and act accordingly.

  o-----> x
  | ---------------------
  | |  0 |  1 |  2 |  3 |
  V ---------------------
  y |  4 |  5 |  6 |  7 |
    ---------------------
    |  8 |  9 | 10 | 11 |
    ---------------------
    | 12 | 13 | 14 | 15 |
    ---------------------

    grid_x = ceil((max(1, pixel_x) / img_w) * grid_cols) - 1.0
    grid_y = ceil((max(1, pixel_y) / img_h) * grid_rows) - 1.0
    cell_id = int(grid_x + (grid_y * grid_cols))

  """
  def __init__(self, grid_rows, grid_cols, image_shape, keypoints):
    assert len(image_shape) == 2
    self.grid_rows = grid_rows
    self.grid_cols = grid_cols
    self.image_shape = image_shape
    self.keypoints = keypoints

    self.cell = [0 for i in range(self.grid_rows * self.grid_cols)]
    for kp in keypoints:
      if hasattr(kp, 'pt'):
        # cv2.KeyPoint
        assert (kp.pt[0] >= 0 and kp.pt[0] <= image_shape[1])
        assert (kp.pt[1] >= 0 and kp.pt[1] <= image_shape[0])
        self.cell[self.cell_index(kp.pt)] += 1
      else:
        # Tuple
        assert (kp[0] >= 0 and kp[0] <= image_shape[1])
        assert (kp[1] >= 0 and kp[1] <= image_shape[0])
        self.cell[self.cell_index(kp)] += 1

  def cell_index(self, pt):
    """ Return cell index based on point `pt` """
    pixel_x, pixel_y = pt
    img_h, img_w = self.image_shape
    grid_x = math.ceil((max(1, pixel_x) / img_w) * self.grid_cols) - 1.0
    grid_y = math.ceil((max(1, pixel_y) / img_h) * self.grid_rows) - 1.0
    cell_id = int(grid_x + (grid_y * self.grid_cols))
    return cell_id

  def count(self, cell_idx):
    """ Return cell count """
    return self.cell[cell_idx]


def grid_detect(detector, image, **kwargs):
  """
  Detect features uniformly using a grid system.
  """
  optflow_mode = kwargs.get('optflow_mode', False)
  max_keypoints = kwargs.get('max_keypoints', 240)
  grid_rows = kwargs.get('grid_rows', 3)
  grid_cols = kwargs.get('grid_cols', 4)
  prev_kps = kwargs.get('prev_kps', [])
  if prev_kps is None:
    prev_kps = []

  # Calculate number of grid cells and max corners per cell
  image_height, image_width = image.shape
  dx = int(math.ceil(float(image_width) / float(grid_cols)))
  dy = int(math.ceil(float(image_height) / float(grid_rows)))
  nb_cells = grid_rows * grid_cols
  max_per_cell = math.floor(max_keypoints / nb_cells)

  # Detect corners in each grid cell
  feature_grid = FeatureGrid(grid_rows, grid_cols, image.shape, prev_kps)
  des_all = []
  kps_all = []

  cell_idx = 0
  for y in range(0, image_height, dy):
    for x in range(0, image_width, dx):
      # Make sure roi width and height are not out of bounds
      w = image_width - x if (x + dx > image_width) else dx
      h = image_height - y if (y + dy > image_height) else dy

      # Detect corners in grid cell
      cs, ce, rs, re = (x, x + w, y, y + h)
      roi_image = image[rs:re, cs:ce]

      kps = None
      des = None
      if optflow_mode:
        detector.setNonmaxSuppression(1)
        kps = detector.detect(roi_image)
        kps = sort_keypoints(kps)

      else:
        kps = detector.detect(roi_image, None)
        kps, des = detector.compute(roi_image, kps)

      # Offset keypoints
      cell_vacancy = max_per_cell - feature_grid.count(cell_idx)
      if cell_vacancy <= 0:
        continue

      limit = min(len(kps), cell_vacancy)
      for i in range(limit):
        kp = kps[i]
        kp.pt = (kp.pt[0] + x, kp.pt[1] + y)
        kps_all.append(kp)
        des_all.append(des[i, :] if optflow_mode is False else None)

      # Update cell_idx
      cell_idx += 1

  # Space out the keypoints
  if optflow_mode:
    kps_all = spread_keypoints(image, kps_all, 20, prev_kps=prev_kps)

  # Debug
  if kwargs.get('debug', False):
    # Setup
    viz = cv2.cvtColor(image, cv2.COLOR_GRAY2RGB)
    kps_grid = FeatureGrid(grid_rows, grid_cols, image.shape, kps_all)

    # Visualization properties
    red = (0, 0, 255)
    yellow = (0, 255, 255)
    linetype = cv2.LINE_AA
    font = cv2.FONT_HERSHEY_SIMPLEX

    # -- Draw horizontal lines
    for x in range(0, image_width, dx):
      cv2.line(viz, (x, 0), (x, image_height), red, 1, linetype)

    # -- Draw vertical lines
    for y in range(0, image_height, dy):
      cv2.line(viz, (0, y), (image_width, y), red, 1, linetype)

    # -- Draw bin numbers
    cell_idx = 0
    for y in range(0, image_height, dy):
      for x in range(0, image_width, dx):
        text = str(kps_grid.count(cell_idx))
        origin = (x + 10, y + 20)
        viz = cv2.putText(viz, text, origin, font, 0.5, red, 1, linetype)

        # text = str(feature_grid.count(cell_idx))
        # origin = (x + 10, y + 20)
        # viz = cv2.putText(viz, text, origin, font, 0.5, yellow, 1, linetype)

        cell_idx += 1

    # -- Draw keypoints
    viz = draw_keypoints(viz, kps_all, color=red)
    viz = draw_keypoints(viz, prev_kps, color=yellow)
    cv2.imshow("viz", viz)
    cv2.waitKey(0)

  # Return
  if optflow_mode:
    return kps_all

  return kps_all, np.array(des_all)


def optflow_track(img_i, img_j, pts_i, **kwargs):
  """
  Track keypoints `pts_i` from image `img_i` to image `img_j` using optical
  flow. Returns a tuple of `(pts_i, pts_j, inliers)` points in image i, j and a
  vector of inliers.
  """
  # Setup
  patch_size = kwargs.get('patch_size', 50)
  max_iter = kwargs.get('max_iter', 100)
  epsilon = kwargs.get('epsilon', 0.001)
  crit = (cv2.TermCriteria_COUNT | cv2.TermCriteria_EPS, max_iter, epsilon)

  # Optical flow settings
  config = {}
  config['winSize'] = (patch_size, patch_size)
  config['maxLevel'] = 3
  config['criteria'] = crit
  config['flags'] = cv2.OPTFLOW_USE_INITIAL_FLOW

  # Track using optical flow
  pts_j = np.array(pts_i)
  track_results = cv2.calcOpticalFlowPyrLK(img_i, img_j, pts_i, pts_j, **config)
  (pts_j, optflow_inliers, _) = track_results

  # Make sure keypoints are within image dimensions
  bound_inliers = []
  img_h, img_w = img_j.shape
  for p in pts_j:
    x_ok = p[0] >= 0 and p[0] <= img_w
    y_ok = p[1] >= 0 and p[1] <= img_h
    if x_ok and y_ok:
      bound_inliers.append(True)
    else:
      bound_inliers.append(False)

  # Update or mark feature as lost
  assert len(pts_i) == optflow_inliers.shape[0]
  assert len(pts_i) == len(bound_inliers)
  inliers = []
  for i in range(len(pts_i)):
    if optflow_inliers[i, 0] and bound_inliers[i]:
      inliers.append(True)
    else:
      inliers.append(False)

  if kwargs.get('debug', False):
    viz_i = draw_keypoints(img_i, pts_i, inliers)
    viz_j = draw_keypoints(img_j, pts_j, inliers)
    viz = cv2.hconcat([viz_i, viz_j])
    cv2.imshow('viz', viz)
    cv2.waitKey(0)

  return (pts_i, pts_j, inliers)


def filter_outliers(pts_i, pts_j, inliers):
  """ Filter outliers """
  pts_out_i = []
  pts_out_j = []
  for n, status in enumerate(inliers):
    if status:
      pts_out_i.append(pts_i[n])
      pts_out_j.append(pts_j[n])

  return (pts_out_i, pts_out_j)


def ransac(pts_i, pts_j, cam_i, cam_j):
  """ RANSAC """
  # Setup
  cam_geom_i = cam_i.data
  cam_geom_j = cam_j.data

  # Undistort points
  pts_i_ud = np.array([cam_geom_i.undistort(cam_i.param, p) for p in pts_i])
  pts_j_ud = np.array([cam_geom_j.undistort(cam_j.param, p) for p in pts_j])

  # Ransac via OpenCV's find fundamental matrix
  method = cv2.FM_RANSAC
  reproj_thresh = 0.75
  confidence = 0.99
  args = [pts_i_ud, pts_j_ud, method, reproj_thresh, confidence]
  _, inliers = cv2.findFundamentalMat(*args)

  return inliers.flatten()


class FeatureTrackerData:
  """
  Feature tracking data *per camera*

  This data structure keeps track of:

  - Image
  - Keypoints
  - Descriptors
  - Feature ids (optional)

  """
  def __init__(self, cam_idx, image, keypoints, feature_ids=None):
    self.cam_idx = cam_idx
    self.image = image
    self.keypoints = list(keypoints)
    self.feature_ids = list(feature_ids)

  def add(self, fid, kp):
    """ Add measurement """
    assert isinstance(fid, int)
    assert hasattr(kp, 'pt')
    self.keypoints.append(kp)
    self.feature_ids.append(fid)
    assert len(self.keypoints) == len(self.feature_ids)

  def update(self, image, fids, kps):
    """ Extend measurements """
    assert len(kps) == len(fids)
    self.image = np.array(image)

    if kps:
      assert hasattr(kps[0], 'pt')

    self.feature_ids.extend(fids)
    self.keypoints.extend(kps)
    assert len(self.keypoints) == len(self.feature_ids)


class FeatureTracker:
  """ Feature tracker """
  def __init__(self):
    # Settings
    self.mode = "TRACK_DEFAULT"
    # self.mode = "TRACK_OVERLAPS"
    # self.mode = "TRACK_INDEPENDENT"

    # Settings
    self.reproj_threshold = 5.0

    # Data
    self.prev_ts = None
    self.frame_idx = 0
    self.detector = cv2.FastFeatureDetector_create(threshold=50)
    self.features_detected = 0
    self.features_tracking = 0
    self.feature_overlaps = {}
    self.prev_mcam_imgs = None
    self.kp_size = 0

    self.cam_idxs = []
    self.cam_params = {}
    self.cam_exts = {}
    self.cam_overlaps = {}
    self.cam_data = {}

  def add_camera(self, cam_idx, cam_params, cam_exts):
    """ Add camera """
    self.cam_idxs.append(cam_idx)
    self.cam_data[cam_idx] = None
    self.cam_params[cam_idx] = cam_params
    self.cam_exts[cam_idx] = cam_exts

  def add_overlap(self, cam_i_idx, cam_j_idx):
    """ Add overlap """
    if cam_i_idx not in self.cam_overlaps:
      self.cam_overlaps[cam_i_idx] = []
    self.cam_overlaps[cam_i_idx].append(cam_j_idx)

  def num_tracking(self):
    """ Return number of features tracking """
    feature_ids = []
    for _, cam_data in self.cam_data.items():
      if cam_data is not None:
        feature_ids.extend(cam_data.feature_ids)
    return len(set(feature_ids))

  def _get_camera_indices(self):
    """ Get camera indices """
    return [cam_idx for cam_idx, _ in self.cam_params]

  def _get_keypoints(self, cam_idx):
    """ Get keypoints observed by camera `cam_idx` """
    keypoints = None
    if self.cam_data[cam_idx] is not None:
      keypoints = self.cam_data[cam_idx].keypoints

    return keypoints

  def _get_feature_ids(self, cam_idx):
    """ Get feature ids observed by camera `cam_idx` """
    feature_ids = None
    if self.cam_data[cam_idx] is not None:
      feature_ids = self.cam_data[cam_idx].feature_ids

    return feature_ids

  def _form_feature_ids(self, nb_kps):
    """ Form list of feature ids for new features to be added """
    self.features_detected += nb_kps
    start_idx = self.features_detected - nb_kps
    end_idx = start_idx + nb_kps
    return list(range(start_idx, end_idx))

  def _triangulate(self, idx_i, idx_j, z_i, z_j):
    """ Triangulate feature """
    # Setup
    cam_i = self.cam_params[idx_i]
    cam_j = self.cam_params[idx_j]
    cam_geom_i = cam_i.data
    cam_geom_j = cam_j.data
    cam_exts_i = self.cam_exts[idx_i]
    cam_exts_j = self.cam_exts[idx_j]

    # Form projection matrices P_i and P_j
    T_BCi = pose2tf(cam_exts_i.param)
    T_BCj = pose2tf(cam_exts_j.param)
    T_CiCj = inv(T_BCi) @ T_BCj
    P_i = pinhole_P(cam_geom_i.proj_params(cam_i.param), eye(4))
    P_j = pinhole_P(cam_geom_j.proj_params(cam_j.param), T_CiCj)

    # Undistort image points z_i and z_j
    x_i = cam_geom_i.undistort(cam_i.param, z_i)
    x_j = cam_geom_j.undistort(cam_j.param, z_j)

    # Linear triangulate
    p_Ci = linear_triangulation(P_i, P_j, x_i, x_j)

    return p_Ci

  def _reproj_filter(self, idx_i, idx_j, pts_i, pts_j):
    """ Filter features by triangulating them via a stereo-pair and see if the
    reprojection error is reasonable """
    assert idx_i != idx_j
    assert len(pts_i) == len(pts_j)

    # Reject outliers based on reprojection error
    reproj_inliers = []
    cam_i = self.cam_params[idx_i]
    cam_geom_i = cam_i.data

    nb_pts = len(pts_i)
    for n in range(nb_pts):
      # Triangulate
      z_i = pts_i[n]
      z_j = pts_j[n]
      p_Ci = self._triangulate(idx_i, idx_j, z_i, z_j)
      if p_Ci[2] < 0.0:
        reproj_inliers.append(False)
        continue

      # Reproject
      status, z_i_hat = cam_geom_i.project(cam_i.param, p_Ci)
      if status is False:
        reproj_inliers.append(False)
      else:
        reproj_error = norm(z_i - z_i_hat)
        if reproj_error > self.reproj_threshold:
          reproj_inliers.append(False)
        else:
          reproj_inliers.append(True)

    return reproj_inliers

  def _add_features(self, cam_idxs, mcam_imgs, cam_kps, fids):
    """ Add features """
    # Pre-check
    assert cam_idxs
    assert all(cam_idx in mcam_imgs for cam_idx in cam_idxs)
    assert all(cam_idx in cam_kps for cam_idx in cam_idxs)

    # Add camera data
    for idx in cam_idxs:
      img = mcam_imgs[idx]
      kps = cam_kps[idx]
      assert len(kps) == len(fids)
      if self.cam_data[idx] is None:
        self.cam_data[idx] = FeatureTrackerData(idx, img, kps, fids)
      else:
        self.cam_data[idx].update(img, fids, kps)

    # Update overlapping features
    if len(cam_idxs) > 1:
      for fid in fids:
        self.feature_overlaps[fid] = 2

  def _update_features(self, cam_idxs, mcam_imgs, cam_kps, fids):
    """ Update features """
    # Pre-check
    assert cam_idxs
    assert all(cam_idx in mcam_imgs for cam_idx in cam_idxs)
    assert all(cam_idx in cam_kps for cam_idx in cam_idxs)

    # Update camera data
    for idx in cam_idxs:
      img = mcam_imgs[idx]
      kps = cam_kps[idx]
      self.cam_data[idx] = FeatureTrackerData(idx, img, kps, fids)

    # # Update lost features
    # fids_out = set(fids)
    # fids_lost = [x for x in fids_in if x not in fids_out]
    # for fid in fids_lost:
    #   # feature overlaps
    #   if fid in self.feature_overlaps:
    #     self.feature_overlaps[fid] -= 1
    #     if self.feature_overlaps[fid] == 0:
    #       del self.feature_overlaps[fid]

  def _detect(self, image, prev_kps=None):
    """ Detect """
    assert image is not None
    kwargs = {'prev_kps': prev_kps, 'optflow_mode': True}
    kps = grid_detect(self.detector, image, **kwargs)
    self.kp_size = kps[0].size if kps else 0
    return kps

  def _detect_overlaps(self, mcam_imgs):
    """ Detect overlapping features """
    # Loop through camera overlaps
    for idx_i, overlaps in self.cam_overlaps.items():
      # Detect keypoints observed from idx_i (primary camera)
      cam_i = self.cam_params[idx_i]
      img_i = mcam_imgs[idx_i]
      prev_kps = self._get_keypoints(idx_i)
      kps_i = self._detect(img_i, prev_kps=prev_kps)
      pts_i = np.array([kp.pt for kp in kps_i], dtype=np.float32)
      fids_new = self._form_feature_ids(len(kps_i))
      if not kps_i:
        continue

      # Track feature from camera idx_i to idx_j (primary to secondary camera)
      for idx_j in overlaps:
        # Optical flow
        img_j = mcam_imgs[idx_j]
        (_, pts_j, optflow_inliers) = optflow_track(img_i, img_j, pts_i)

        # RANSAC
        ransac_inliers = []
        if len(kps_i) < 10:
          ransac_inliers = np.array([True for _, _ in enumerate(kps_i)])
        else:
          cam_j = self.cam_params[idx_j]
          ransac_inliers = ransac(pts_i, pts_j, cam_i, cam_j)

        # Reprojection filter
        reproj_inliers = self._reproj_filter(idx_i, idx_j, pts_i, pts_j)

        # Filter outliers
        inliers = optflow_inliers & ransac_inliers & reproj_inliers
        kps_j = [cv2.KeyPoint(p[0], p[1], self.kp_size) for p in pts_j]
        fids = []
        cam_kps = {idx_i: [], idx_j: []}
        for i, inlier in enumerate(inliers):
          if inlier:
            fids.append(fids_new[i])
            cam_kps[idx_i].append(kps_i[i])
            cam_kps[idx_j].append(kps_j[i])

        # Add features
        cam_idxs = [idx_i, idx_j]
        cam_imgs = {idx_i: img_i, idx_j: img_j}
        self._add_features(cam_idxs, cam_imgs, cam_kps, fids)

  def _detect_nonoverlaps(self, mcam_imgs):
    """ Detect non-overlapping features """
    for idx in self.cam_params:
      # Detect keypoints
      img = mcam_imgs[idx]
      prev_kps = self._get_keypoints(idx)
      kps = self._detect(img, prev_kps=prev_kps)
      if not kps:
        return

      # Add features
      fids = self._form_feature_ids(len(kps))
      self._add_features([idx], {idx: img}, {idx: kps}, fids)

  def _detect_new(self, mcam_imgs):
    """ Detect new features """

    # Detect new features
    if self.mode == "TRACK_DEFAULT":
      self._detect_overlaps(mcam_imgs)
      self._detect_nonoverlaps(mcam_imgs)
    elif self.mode == "TRACK_OVERLAPS":
      self._detect_overlaps(mcam_imgs)
    elif self.mode == "TRACK_INDEPENDENT":
      self._detect_nonoverlaps(mcam_imgs)
    else:
      raise RuntimeError("Invalid FeatureTracker mode [%s]!" % self.mode)

  def _track_through_time(self, mcam_imgs, cam_idx):
    """ Track features through time """

    # Setup images
    img_km1 = self.prev_mcam_imgs[cam_idx]
    img_k = mcam_imgs[cam_idx]

    # Setup keypoints and feature_ids
    kps_km1 = self._get_keypoints(cam_idx)
    feature_ids = self._get_feature_ids(cam_idx)
    pts_km1 = np.array([kp.pt for kp in kps_km1], dtype=np.float32)

    # Optical flow
    (pts_km1, pts_k, optflow_inliers) = optflow_track(img_km1, img_k, pts_km1)

    # RANSAC
    ransac_inliers = []
    if len(kps_km1) < 10:
      ransac_inliers = np.array([True for _, _ in enumerate(kps_km1)])
    else:
      cam = self.cam_params[cam_idx]
      ransac_inliers = ransac(pts_km1, pts_k, cam, cam)

    # Form inliers list
    optflow_inliers = np.array(optflow_inliers)
    ransac_inliers = np.array(ransac_inliers)
    inliers = optflow_inliers & ransac_inliers

    return (pts_km1, pts_k, feature_ids, inliers)

  def _track_stereo(self, mcam_imgs, idx_i, idx_j, pts_i):
    """ Track feature through stereo-pair """
    # Optical flow
    img_i = mcam_imgs[idx_i]
    img_j = mcam_imgs[idx_j]
    (pts_i, pts_j, optflow_inliers) = optflow_track(img_i, img_j, pts_i)

    # RANSAC
    cam_i = self.cam_params[idx_i]
    cam_j = self.cam_params[idx_j]
    ransac_inliers = ransac(pts_i, pts_j, cam_i, cam_j)

    # Reject outliers based on reprojection error
    reproj_inliers = self._reproj_filter(idx_i, idx_j, pts_i, pts_j)

    # Logical AND optflow_inliers and reproj_inliers
    ransac_inliers = np.array(ransac_inliers)
    optflow_inliers = np.array(optflow_inliers)
    reproj_inliers = np.array(reproj_inliers)
    inliers = optflow_inliers & ransac_inliers & reproj_inliers

    return (pts_i, pts_j, inliers)

  def _track_features(self, mcam_imgs):
    """ Track features """
    # Track features in each camera
    for idx in self.cam_idxs:
      # Track through time
      track_results = self._track_through_time(mcam_imgs, idx)
      (_, pts_k, fids_old, inliers) = track_results

      fids = []
      kps = []
      for i, inlier in enumerate(inliers):
        if inlier:
          pt = pts_k[i]
          fids.append(fids_old[i])
          kps.append(cv2.KeyPoint(pt[0], pt[1], self.kp_size))

      # Update features
      cam_idxs = [idx]
      cam_imgs = {idx: mcam_imgs[idx]}
      cam_kps = {idx: kps}
      self._update_features(cam_idxs, cam_imgs, cam_kps, fids)

  def update(self, ts, mcam_imgs):
    """ Update Feature Tracker """
    # Track features
    if self.frame_idx == 0:
      self._detect_new(mcam_imgs)
      self.features_tracking = self.num_tracking()
    else:
      self._track_features(mcam_imgs)
      if (self.num_tracking() / self.features_tracking) < 0.8:
        self._detect_new(mcam_imgs)

    # Update
    self.frame_idx += 1
    self.prev_ts = ts
    self.prev_mcam_imgs = mcam_imgs

    return self.cam_data


def visualize_tracking(ft_data):
  """ Visualize feature tracking data """
  viz = []

  radius = 4
  green = (0, 255, 0)
  yellow = (0, 255, 255)
  thickness = 1
  linetype = cv2.LINE_AA

  # Find overlaps
  fids = {}
  feature_overlaps = set()
  for _, cam_data in ft_data.items():
    for n, _ in enumerate(cam_data.keypoints):
      fid = cam_data.feature_ids[n]
      fids[fid] = (fids[fid] + 1) if fid in fids else 1

      if fids[fid] > 1:
        feature_overlaps.add(fid)

  # Draw features being tracked in each camera
  for _, cam_data in ft_data.items():
    img = cam_data.image
    cam_viz = cv2.cvtColor(img, cv2.COLOR_GRAY2RGB)

    for n, kp in enumerate(cam_data.keypoints):
      fid = cam_data.feature_ids[n]
      color = green if fid in feature_overlaps else yellow
      p = (int(kp.pt[0]), int(kp.pt[1])) if hasattr(kp, 'pt') else kp
      cv2.circle(cam_viz, p, radius, color, thickness, lineType=linetype)

    viz.append(cam_viz)

  return cv2.hconcat(viz)


# STATE-ESTIMATOR #############################################################


class KeyFrame:
  """ Key Frame """
  def __init__(self, ts, images, pose, vision_factors):
    self.ts = ts
    self.images = images
    self.pose = pose
    self.vision_factors = vision_factors


class Tracker:
  """ Tracker """
  def __init__(self, feature_tracker):
    # Feature tracker
    self.feature_tracker = feature_tracker

    # Flags
    self.imu_started = False
    self.cams_started = False

    # Data
    self.graph = FactorGraph()
    self.pose_init = None

    self.imu_buf = ImuBuffer()
    self.imu_params = None

    self.cam_params = {}
    self.cam_geoms = {}
    self.cam_exts = {}
    self.features = {}
    self.keyframes = []

    # Settings
    self.window_size = 10

  def nb_cams(self):
    """ Return number of cameras """
    return len(self.cam_params)

  def nb_keyframes(self):
    """ Return number of keyframes """
    return len(self.keyframes)

  def nb_features(self):
    """ Return number of keyframes """
    return len(self.features)

  def add_imu(self, imu_params):
    """ Add imu """
    self.imu_params = imu_params

  def add_camera(self, cam_idx, cam_params, cam_exts):
    """ Add camera """
    self.cam_params[cam_idx] = cam_params
    self.cam_geoms[cam_idx] = cam_params.data
    self.cam_exts[cam_idx] = cam_exts
    self.graph.add_param(cam_params)
    self.graph.add_param(cam_exts)
    self.feature_tracker.add_camera(cam_idx, cam_params, cam_exts)

  def add_overlap(self, cam_i, cam_j):
    """ Add overlap """
    self.feature_tracker.add_overlap(cam_i, cam_j)

  def set_initial_pose(self, T_WB):
    """ Set initial pose """
    assert self.pose_init is None
    self.pose_init = T_WB

  def inertial_callback(self, ts, acc, gyr):
    """ Inertial callback """
    if self.imu_params is None:
      raise RuntimeError("Forgot to add imu to tracker?")
    self.imu_buf.add(ts, acc, gyr)
    self.imu_started = True

  def _triangulate(self, cam_i, cam_j, z_i, z_j, T_WB):
    """ Triangulate feature """
    # Setup
    cam_params_i = self.cam_params[cam_i]
    cam_params_j = self.cam_params[cam_j]
    cam_geom_i = cam_params_i.data
    cam_geom_j = cam_params_j.data
    cam_exts_i = self.cam_exts[cam_i]
    cam_exts_j = self.cam_exts[cam_j]

    # Form projection matrices P_i and P_j
    T_BCi = pose2tf(cam_exts_i.param)
    T_BCj = pose2tf(cam_exts_j.param)
    T_CiCj = inv(T_BCi) @ T_BCj
    P_i = pinhole_P(cam_geom_i.proj_params(cam_params_i.param), eye(4))
    P_j = pinhole_P(cam_geom_j.proj_params(cam_params_j.param), T_CiCj)

    # Undistort image points z_i and z_j
    x_i = cam_geom_i.undistort(cam_params_i.param, z_i)
    x_j = cam_geom_j.undistort(cam_params_j.param, z_j)

    # Linear triangulate
    p_Ci = linear_triangulation(P_i, P_j, x_i, x_j)
    if p_Ci[2] < 0.0:
      return None

    # Transform feature from camera frame to world frame
    T_BCi = pose2tf(self.cam_exts[cam_i].param)
    p_W = tf_point(T_WB @ T_BCi, p_Ci)
    return p_W

  def _add_pose(self, ts, T_WB):
    """
    Add pose

    Args:

      T_WB (np.array): Body pose in world frame

    """
    pose = pose_setup(ts, T_WB)
    self.graph.add_param(pose)
    return pose

  def _get_last_pose(self):
    """ Get last pose """
    return pose2tf(self.keyframes[-1].pose.param)

  def _add_feature(self, fid, ts, cam_idx, kp):
    """
    Add feature

    Args:

      fid (int): Feature id
      ts (int): Timestamp
      cam_idx (int): Camera index
      kp (cv2.KeyPoint): Key point

    """
    assert hasattr(kp, 'pt')
    self.features[fid] = feature_setup(zeros((3,)))
    self.features[fid].data.update(ts, cam_idx, kp.pt)
    feature_pid = self.graph.add_param(self.features[fid])
    return feature_pid

  def _update_feature(self, fid, ts, cam_idx, kp, T_WB):
    """
    Update feature

    Args:

      fid (int): Feature id
      ts (int): Timestamp
      cam_idx (int): Camera index
      kp (cv2.KeyPoint): Key point
      T_WB (np.array): Body pose in world frame

    """
    # Update feature
    self.features[fid].data.update(ts, cam_idx, kp.pt)

    # Initialize overlapping features
    has_inited = self.features[fid].data.initialized()
    has_overlap = self.features[fid].data.has_overlap(ts)
    if has_inited is False and has_overlap is True:
      overlaps = self.features[fid].data.get_overlaps(ts)
      cam_i, z_i = overlaps[0]
      cam_j, z_j = overlaps[1]
      p_W = self._triangulate(cam_i, cam_j, z_i, z_j, T_WB)
      if p_W is not None:
        self.features[fid].param = p_W
        self.features[fid].data.set_initialized()

  def _process_features(self, ts, ft_data, pose):
    """ Process features

    Args:

      ts (int): Timestamp
      ft_data (Dict[int, FeatureTrackerData]): Multi-camera feature tracker data
      pose (StateVariable): Body pose in world frame

    """
    # Add or update feature
    T_WB = pose2tf(pose.param)

    for cam_idx, cam_data in ft_data.items():
      for fid, kp in zip(cam_data.feature_ids, cam_data.keypoints):
        if fid not in self.features:
          self._add_feature(fid, ts, cam_idx, kp)
        else:
          self._update_feature(fid, ts, cam_idx, kp, T_WB)

  def _add_keyframe(self, ts, mcam_imgs, ft_data, pose):
    """
    Add keyframe

    Args:

      ts (int): Timestamp
      mcam_imgs (Dict[int, np.array]): Multi-camera images
      ft_data (Dict[int, FeatureTrackerData]): Multi-camera features
      pose (Pose): Body pose in world frame

    """
    vision_factors = []

    for cam_idx, cam_data in ft_data.items():
      # camera params, geometry and extrinsics
      cam_params = self.cam_params[cam_idx]
      cam_geom = self.cam_geoms[cam_idx]
      cam_exts = self.cam_exts[cam_idx]

      # Form vision factors
      for fid, kp in zip(cam_data.feature_ids, cam_data.keypoints):
        feature = self.features[fid]
        if feature.data.initialized() is False:
          continue

        # Form vision factor
        param_ids = []
        param_ids.append(pose.param_id)
        param_ids.append(cam_exts.param_id)
        param_ids.append(feature.param_id)
        param_ids.append(cam_params.param_id)
        factor = VisionFactor(cam_geom, param_ids, kp.pt)
        vision_factors.append(factor)
        self.graph.add_factor(factor)

    # Form keyframe
    self.keyframes.append(KeyFrame(ts, mcam_imgs, pose, vision_factors))

  def _pop_old_keyframe(self):
    """ Pop old keyframe """
    # Remove pose parameter and vision factors
    kf = self.keyframes[0]
    self.graph.remove_param(kf.pose)
    for factor in kf.vision_factors:
      self.graph.remove_factor(factor)

    # Pop the front of the queue
    self.keyframes.pop(0)

  def _filter_keyframe_factors(self, filter_from=0):
    """ Filter keyframe factors """
    removed = 0

    for kf in self.keyframes[filter_from:]:
      # Calculate reprojection error
      reproj_errors = []
      for factor in list(kf.vision_factors):
        # factor_params = self.graph._get_factor_params(factor)
        params = [self.graph.params[pid].param for pid in factor.param_ids]
        r, _ = factor.eval(params)
        reproj_errors.append(norm(r))

      # Filter factors
      threshold = 3.0 * np.std(reproj_errors)
      filtered_factors = []

      for reproj_error, factor in zip(reproj_errors, kf.vision_factors):
        if reproj_error >= threshold:
          self.graph.remove_factor(factor)
          removed += 1
        else:
          filtered_factors.append(factor)
      kf.vision_factors = filtered_factors

  def vision_callback(self, ts, mcam_imgs):
    """
    Vision callback

    Args:

      ts (int): Timestamp
      mcam_imgs (Dict[int, np.array]): Multi-camera images

    """
    assert self.pose_init is not None

    # Has IMU?
    if self.imu_params is not None and self.imu_started is False:
      return

    # Perform feature tracking
    ft_data = self.feature_tracker.update(ts, mcam_imgs)

    # Add pose
    pose = None
    if self.nb_keyframes() == 0:
      pose = self._add_pose(ts, self.pose_init)
    else:
      T_WB = self._get_last_pose()
      pose = self._add_pose(ts, T_WB)

    # Process features, add keyframe and solve
    self._process_features(ts, ft_data, pose)
    self._add_keyframe(ts, mcam_imgs, ft_data, pose)

    if self.nb_keyframes() != 1:
      self.graph.solve(True)
      self._filter_keyframe_factors()

    if len(self.keyframes) > self.window_size:
      self._pop_old_keyframe()

    errors = self.graph.get_reproj_errors()
    print(f"reproj_error:", end=" [")
    print(f"mean: {np.mean(errors):.2f}", end=", ")
    print(f"median: {np.median(errors):.2f}", end=", ")
    print(f"rms: {rmse(errors):.2f}", end=", ")
    print(f"max: {np.max(errors):.2f}", end="]\n")
    print(f"nb_keyframes: {self.nb_keyframes()}")
    print()


###############################################################################
# CALIBRATION
###############################################################################


class AprilGrid:
  """ AprilGrid """
  def __init__(self, **kwargs):
    self.tag_rows = kwargs.get("tag_rows", 6)
    self.tag_cols = kwargs.get("tag_cols", 6)
    self.tag_size = kwargs.get("tag_size", 0.088)
    self.tag_spacing = kwargs.get("tag_spacing", 0.3)
    self.nb_tags = self.tag_rows * self.tag_cols
    self.ts = None
    self.data = {}

  @staticmethod
  def load(csv_file):
    """ Load AprilGrid """
    # Load csv file
    csv_data = pandas.read_csv(csv_file)
    if csv_data.shape[0] == 0:
      return None

    # AprilGrid properties
    ts = csv_data['#ts'][0]
    tag_rows = csv_data['tag_rows'][0]
    tag_cols = csv_data['tag_cols'][0]
    tag_size = csv_data['tag_size'][0]
    tag_spacing = csv_data['tag_spacing'][0]

    # AprilGrid measurements
    tag_indices = csv_data['tag_id']
    corner_indices = csv_data['corner_idx']
    kps = np.array([csv_data['kp_x'], csv_data['kp_y']]).T

    # Form AprilGrid
    grid_conf = {
        "tag_rows": tag_rows,
        "tag_cols": tag_cols,
        "tag_size": tag_size,
        "tag_spacing": tag_spacing
    }
    grid = AprilGrid(**grid_conf)
    for tag_id, corner_idx, kp in zip(tag_indices, corner_indices, kps):
      grid.add_keypoint(ts, tag_id, corner_idx, kp)

    return grid

  def get_object_point(self, tag_id, corner_idx):
    """ Form object point """
    # Calculate the AprilGrid index using tag id
    [i, j] = self.get_grid_index(tag_id)

    # Calculate the x and y of the tag origin (bottom left corner of tag)
    # relative to grid origin (bottom left corner of entire grid)
    x = j * (self.tag_size + self.tag_size * self.tag_spacing)
    y = i * (self.tag_size + self.tag_size * self.tag_spacing)

    # Corners from bottom left in counter-clockwise fashion
    if corner_idx == 0:
      # Bottom left
      return np.array([x, y, 0])
    elif corner_idx == 1:
      # Bottom right
      return np.array([x + self.tag_size, y, 0])
    elif corner_idx == 2:
      # Top right
      return np.array([x + self.tag_size, y + self.tag_size, 0])
    elif corner_idx == 3:
      # Top left
      return np.array([x, y + self.tag_size, 0])

    raise RuntimeError(f"Invalid tag_id[{tag_id}] corner_idx[{corner_idx}]!")

  def get_object_points(self):
    """ Form object points """
    object_points = []
    for tag_id in range(self.nb_tags):
      for corner_idx in range(4):
        object_points.append(self.get_object_point(tag_id, corner_idx))
    return np.array(object_points)

  def get_center(self):
    """ Calculate center of aprilgrid """
    x = (self.tag_cols / 2.0) * self.tag_size
    x += ((self.tag_cols / 2.0) - 1) * self.tag_spacing * self.tag_size
    x += 0.5 * self.tag_spacing * self.tag_size

    y = (self.tag_rows / 2.0) * self.tag_size
    y += ((self.tag_rows / 2.0) - 1) * self.tag_spacing * self.tag_size
    y += 0.5 * self.tag_spacing * self.tag_size

    return np.array([x, y])

  def get_grid_index(self, tag_id):
    """ Calculate grid index from tag id """
    assert tag_id < (self.nb_tags) and tag_id >= 0
    i = int(tag_id / self.tag_cols)
    j = int(tag_id % self.tag_cols)
    return (i, j)

  def add_keypoint(self, ts, tag_id, corner_idx, kp):
    """ Add keypoint """
    self.ts = ts
    if tag_id not in self.data:
      self.data[tag_id] = {}
    self.data[tag_id][corner_idx] = kp

  def remove_keypoint(self, tag_id, corner_idx):
    """ Remove keypoint """
    assert tag_id in self.data
    assert corner_idx in self.data[tag_id]
    del self.data[tag_id][corner_idx]

  def add_tag_data(self, ts, tag_data):
    """ Add tag data """
    for (tag_id, corner_idx, kp_x, kp_y) in tag_data:
      self.add_keypoint(ts, tag_id, corner_idx, np.array([kp_x, kp_y]))

  def get_measurements(self):
    """ Get measurements """
    data = []
    for tag_id, tag_data in self.data.items():
      for corner_idx, kp in tag_data.items():
        obj_point = self.get_object_point(tag_id, corner_idx)
        data.append((tag_id, corner_idx, obj_point, kp))

    return data

  def solvepnp(self, cam_params):
    """ Estimate relative transform between camera and aprilgrid """
    # Check if we actually have data to work with
    if not self.data:
      return None

    # Create object points (counter-clockwise, from bottom left)
    cam_geom = cam_params.data
    obj_pts = []
    img_pts = []
    for (_, _, r_FFi, z) in self.get_measurements():
      img_pts.append(cam_geom.undistort(cam_params.param, z))
      obj_pts.append(r_FFi)
    obj_pts = np.array(obj_pts)
    img_pts = np.array(img_pts)

    # Solve pnp
    K = pinhole_K(cam_params.param[0:4])
    D = np.array([0.0, 0.0, 0.0, 0.0])
    flags = cv2.SOLVEPNP_ITERATIVE
    _, rvec, tvec = cv2.solvePnP(obj_pts, img_pts, K, D, False, flags=flags)

    # Form relative tag pose as a 4x4 transform matrix
    C, _ = cv2.Rodrigues(rvec)
    r = tvec.flatten()
    T_CF = tf(C, r)

    return T_CF

  def plot(self, ax, T_WF):
    """ Plot """
    obj_pts = self.get_object_points()
    for row_idx in range(obj_pts.shape[0]):
      r_FFi = obj_pts[row_idx, :]
      r_WFi = tf_point(T_WF, r_FFi)
      ax.plot(r_WFi[0], r_WFi[1], r_WFi[2], 'r.')


def calib_generate_poses(calib_target, **kwargs):
  """ Generate calibration poses infront of the calibration target """
  # Pose settings
  x_range = kwargs.get('x_range', np.linspace(-0.3, 0.3, 5))
  y_range = kwargs.get('y_range', np.linspace(-0.3, 0.3, 5))
  z_range = kwargs.get('z_range', np.linspace(0.3, 0.5, 5))

  # Generate camera positions infront of the calib target r_FC
  calib_center = np.array([*calib_target.get_center(), 0.0])
  cam_pos = []
  pos_idx = 0
  for x in x_range:
    for y in y_range:
      for z in z_range:
        r_FC = np.array([x, y, z]) + calib_center
        cam_pos.append(r_FC)
        pos_idx += 1

  # For each position create a camera pose that "looks at" the calib
  # center in the target frame, T_FC.
  return [lookat(r_FC, calib_center) for r_FC in cam_pos]


def calib_generate_random_poses(calib_target, **kwargs):
  """ Generate random calibration poses infront of the calibration target """
  # Settings
  nb_poses = kwargs.get('nb_poses', 30)
  att_range = kwargs.get('att_range', [deg2rad(-10.0), deg2rad(10.0)])
  x_range = kwargs.get('x_range', [-0.5, 0.5])
  y_range = kwargs.get('y_range', [-0.5, 0.5])
  z_range = kwargs.get('z_range', [0.5, 0.7])

  # For each position create a camera pose that "looks at" the calibration
  # center in the target frame, T_FC.
  calib_center = np.array([*calib_target.get_center(), 0.0])
  poses = []

  for _ in range(nb_poses):
    # Generate random pose
    x = np.random.uniform(x_range[0], x_range[1])
    y = np.random.uniform(y_range[0], y_range[1])
    z = np.random.uniform(z_range[0], z_range[1])
    r_FC = calib_center + np.array([x, y, z])
    T_FC = lookat(r_FC, calib_center)

    # Perturb the pose a little so it doesn't look at the center directly
    yaw = np.random.uniform(*att_range)
    pitch = np.random.uniform(*att_range)
    roll = np.random.uniform(*att_range)
    C_perturb = euler321(yaw, pitch, roll)
    r_perturb = zeros((3,))
    T_perturb = tf(C_perturb, r_perturb)

    poses.append(T_FC @ T_perturb)

  return poses


class CalibView:
  """ Calibration View """
  def __init__(self, pose, cam_params, cam_exts, grid):
    self.ts = grid.ts
    self.pose = pose
    self.cam_idx = cam_params.data.cam_idx
    self.cam_params = cam_params
    self.cam_geom = cam_params.data
    self.cam_exts = cam_exts
    self.grid = grid
    self.factors = []

    pids = [pose.param_id, cam_exts.param_id, cam_params.param_id]
    for grid_data in grid.get_measurements():
      self.factors.append(CalibVisionFactor(self.cam_geom, pids, grid_data))

  def get_reproj_errors(self):
    """ Get reprojection errors """
    reproj_errors = []

    factor_params = [self.pose, self.cam_exts, self.cam_params]
    for factor in self.factors:
      reproj_error = factor.get_reproj_error(*factor_params)
      if reproj_error is not None:
        reproj_errors.append(reproj_error)

    return reproj_errors


class Calibrator:
  """ Calibrator """
  def __init__(self):
    # Parameters
    self.cam_geoms = {}
    self.cam_params = {}
    self.cam_exts = {}
    self.imu_params = None

    # Data
    self.graph = FactorGraph()
    self.poses = {}
    self.calib_views = {}

  def get_num_cams(self):
    """ Return number of cameras """
    return len(self.cam_params)

  def get_num_views(self):
    """ Return number of views """
    return len(self.calib_views)

  def add_camera(self, cam_idx, cam_res, proj_model, dist_model):
    """ Add camera """
    fx = focal_length(cam_res[0], 90.0)
    fy = focal_length(cam_res[1], 90.0)
    cx = cam_res[0] / 2.0
    cy = cam_res[1] / 2.0
    params = [fx, fy, cx, cy, 0.0, 0.0, 0.0, 0.0]
    args = [cam_idx, cam_res, proj_model, dist_model, params]
    cam_params = camera_params_setup(*args)

    fix_exts = True if cam_idx == 0 else False
    self.cam_geoms[cam_idx] = cam_params.data
    self.cam_params[cam_idx] = cam_params
    self.cam_exts[cam_idx] = extrinsics_setup(eye(4), fix=fix_exts)

    self.graph.add_param(self.cam_params[cam_idx])
    self.graph.add_param(self.cam_exts[cam_idx])

  def add_imu(self, imu_params):
    """ Add imu """
    self.imu_params = imu_params

  def add_camera_view(self, ts, cam_idx, grid):
    """ Add camera view """
    # Estimate relative pose T_BF
    cam_params = self.cam_params[cam_idx]
    cam_exts = self.cam_exts[cam_idx]
    T_CiF = grid.solvepnp(cam_params)
    T_BCi = pose2tf(cam_exts.param)
    T_BF = T_BCi @ T_CiF
    self.poses[ts] = pose_setup(ts, T_BF)

    # CalibView
    self.graph.add_param(self.poses[ts])
    self.calib_views[ts] = CalibView(self.poses[ts], cam_params, cam_exts, grid)
    for factor in self.calib_views[ts].factors:
      self.graph.add_factor(factor)

    # # Solve
    # if len(self.calib_views) >= 5:
    #   self.graph.solver_max_iter = 10
    #   self.graph.solve(True)
    #
    #   # Calculate reprojection error
    #   reproj_errors = self.graph.get_reproj_errors()
    #   print(f"nb_reproj_errors: {len(reproj_errors)}")
    #   print(f"rms_reproj_errors: {rmse(reproj_errors):.4f} [px]")
    #   print()
    #   # plt.hist(reproj_errors)
    #   # plt.show()

  def solve(self):
    """ Solve """
    self.graph.solver_max_iter = 30
    self.graph.solve(True)

    reproj_errors = self.graph.get_reproj_errors()
    print(f"nb_cams: {self.get_num_cams()}")
    print(f"nb_views: {self.get_num_views()}")
    print(f"nb_reproj_errors: {len(reproj_errors)}")
    print(f"rms_reproj_errors: {rmse(reproj_errors):.4f} [px]")
    sys.stdout.flush()


###############################################################################
# SIMULATION
###############################################################################

# UTILS #######################################################################


def create_3d_features(x_bounds, y_bounds, z_bounds, nb_features):
  """ Create 3D features randomly """
  features = zeros((nb_features, 3))
  for i in range(nb_features):
    features[i, 0] = random.uniform(*x_bounds)
    features[i, 1] = random.uniform(*y_bounds)
    features[i, 2] = random.uniform(*z_bounds)
  return features


def create_3d_features_perimeter(origin, dim, nb_features):
  """ Create 3D features in a square """
  assert len(origin) == 3
  assert len(dim) == 3
  assert nb_features > 0

  # Dimension of the outskirt
  w, l, h = dim

  # Features per side
  nb_fps = int(nb_features / 4.0)

  # Features in the east side
  x_bounds = [origin[0] - w, origin[0] + w]
  y_bounds = [origin[1] + l, origin[1] + l]
  z_bounds = [origin[2] - h, origin[2] + h]
  east = create_3d_features(x_bounds, y_bounds, z_bounds, nb_fps)

  # Features in the north side
  x_bounds = [origin[0] + w, origin[0] + w]
  y_bounds = [origin[1] - l, origin[1] + l]
  z_bounds = [origin[2] - h, origin[2] + h]
  north = create_3d_features(x_bounds, y_bounds, z_bounds, nb_fps)

  # Features in the west side
  x_bounds = [origin[0] - w, origin[0] + w]
  y_bounds = [origin[1] - l, origin[1] - l]
  z_bounds = [origin[2] - h, origin[2] + h]
  west = create_3d_features(x_bounds, y_bounds, z_bounds, nb_fps)

  # Features in the south side
  x_bounds = [origin[0] - w, origin[0] - w]
  y_bounds = [origin[1] - l, origin[1] + l]
  z_bounds = [origin[2] - h, origin[2] + h]
  south = create_3d_features(x_bounds, y_bounds, z_bounds, nb_fps)

  # Stack features and return
  return np.block([[east], [north], [west], [south]])


# SIMULATION ##################################################################


class SimCameraFrame:
  """ Sim camera frame """
  def __init__(self, ts, cam_idx, camera, T_WCi, features):
    assert T_WCi.shape == (4, 4)
    assert features.shape[0] > 0
    assert features.shape[1] == 3

    self.ts = ts
    self.cam_idx = cam_idx
    self.T_WCi = T_WCi
    self.cam_geom = camera.data
    self.cam_params = camera.param
    self.feature_ids = []
    self.measurements = []

    # Simulate camera frame
    nb_points = features.shape[0]
    T_CiW = tf_inv(self.T_WCi)

    for i in range(nb_points):
      # Project point from world frame to camera frame
      p_W = features[i, :]
      p_C = tf_point(T_CiW, p_W)
      status, z = self.cam_geom.project(self.cam_params, p_C)
      if status:
        self.measurements.append(z)
        self.feature_ids.append(i)

  def num_measurements(self):
    """ Return number of measurements """
    return len(self.measurements)

  def draw_measurements(self):
    """ Returns camera measurements in an image """
    kps = [kp for kp in self.measurements]
    img_w, img_h = self.cam_geom.resolution
    img = np.zeros((img_h, img_w), dtype=np.uint8)
    return draw_keypoints(img, kps)


class SimCameraData:
  """ Sim camera data """
  def __init__(self, cam_idx, camera, features):
    self.cam_idx = cam_idx
    self.camera = camera
    self.features = features
    self.timestamps = []
    self.poses = {}
    self.frames = {}


class SimImuData:
  """ Sim imu data """
  def __init__(self, imu_idx):
    self.imu_idx = imu_idx
    self.timestamps = []
    self.poses = {}
    self.vel = {}
    self.acc = {}
    self.gyr = {}

  def form_imu_buffer(self, start_idx, end_idx):
    """ Form ImuBuffer """
    imu_ts = self.timestamps[start_idx:end_idx]
    imu_acc = []
    imu_gyr = []
    for ts in self.timestamps:
      imu_acc.append(self.acc[ts])
      imu_gyr.append(self.gyr[ts])

    return ImuBuffer(imu_ts, imu_acc, imu_gyr)


class SimData:
  """ Sim data """
  def __init__(self, circle_r, circle_v, **kwargs):
    # Settings
    self.circle_r = circle_r
    self.circle_v = circle_v
    self.cam_rate = 10.0
    self.imu_rate = 200.0
    self.nb_features = 200

    # Trajectory data
    self.g = np.array([0.0, 0.0, 9.81])
    self.circle_dist = 2.0 * pi * circle_r
    self.time_taken = self.circle_dist / self.circle_v
    self.w = -2.0 * pi * (1.0 / self.time_taken)
    self.theta_init = pi
    self.yaw_init = pi / 2.0
    self.features = self._setup_features()

    # Simulate IMU
    self.imu0_data = None
    if kwargs.get("sim_imu", True):
      self.imu0_data = self._sim_imu(0)

    # Simulate camera
    self.mcam_data = {}
    self.cam_exts = {}
    if kwargs.get("sim_cams", True):
      # -- cam0
      self.cam0_params = self._setup_camera(0)
      C_BC0 = euler321(*deg2rad([-90.0, 0.0, -90.0]))
      r_BC0 = np.array([0.0, 0.0, 0.0])
      self.T_BC0 = tf(C_BC0, r_BC0)
      self.mcam_data[0] = self._sim_cam(0, self.cam0_params, self.T_BC0)
      self.cam_exts[0] = extrinsics_setup(self.T_BC0)
      # -- cam1
      self.cam1_params = self._setup_camera(1)
      C_BC1 = euler321(*deg2rad([-90.0, 0.0, -90.0]))
      r_BC1 = np.array([0.0, 0.0, 0.0])
      self.T_BC1 = tf(C_BC1, r_BC1)
      # -- Multicam data
      self.mcam_data[1] = self._sim_cam(1, self.cam1_params, self.T_BC1)
      self.cam_exts[1] = extrinsics_setup(self.T_BC1)

    # Timeline
    self.timeline = self._form_timeline()

  def get_camera_data(self, cam_idx):
    """ Get camera data """
    return self.mcam_data[cam_idx]

  def get_camera_params(self, cam_idx):
    """ Get camera parameters """
    return self.mcam_data[cam_idx].camera

  def get_camera_geometry(self, cam_idx):
    """ Get camera geometry """
    return self.mcam_data[cam_idx].camera.data

  def get_camera_extrinsics(self, cam_idx):
    """ Get camera extrinsics """
    return self.cam_exts[cam_idx]

  def plot_scene(self):
    """ Plot 3D Scene """
    # Setup
    plt.figure()
    ax = plt.axes(projection='3d')

    # Plot features
    features = self.features
    ax.scatter3D(features[:, 0], features[:, 1], features[:, 2])

    # Plot camera frames
    idx = 0
    for _, T_WB in self.imu0_data.poses.items():
      if idx % 100 == 0:
        T_BC0 = pose2tf(self.cam_exts[0].param)
        T_BC1 = pose2tf(self.cam_exts[1].param)
        plot_tf(ax, T_WB @ T_BC0)
        plot_tf(ax, T_WB @ T_BC1)
      if idx > 3000:
        break
      idx += 1

    # Show
    plt.show()

  @staticmethod
  def create_or_load(circle_r, circle_v, pickle_path):
    """ Create or load SimData """
    sim_data = None

    if os.path.exists(pickle_path):
      with open(pickle_path, 'rb') as f:
        sim_data = pickle.load(f)
    else:
      sim_data = SimData(circle_r, circle_v)
      with open(pickle_path, 'wb') as f:
        pickle.dump(sim_data, f)
        f.flush()

    return sim_data

  @staticmethod
  def _setup_camera(cam_idx):
    """ Setup camera """
    res = [640, 480]
    fov = 120.0
    fx = focal_length(res[0], fov)
    fy = focal_length(res[0], fov)
    cx = res[0] / 2.0
    cy = res[1] / 2.0

    proj_model = "pinhole"
    dist_model = "radtan4"
    proj_params = [fx, fy, cx, cy]
    dist_params = [0.0, 0.0, 0.0, 0.0]
    params = np.block([*proj_params, *dist_params])

    return camera_params_setup(cam_idx, res, proj_model, dist_model, params)

  def _setup_features(self):
    """ Setup features """
    origin = [0, 0, 0]
    dim = [self.circle_r * 2.0, self.circle_r * 2.0, self.circle_r * 1.5]
    return create_3d_features_perimeter(origin, dim, self.nb_features)

  def _sim_imu(self, imu_idx):
    """ Simulate IMU """
    sim_data = SimImuData(imu_idx)

    ts = 0
    dt_ns = sec2ts(1.0 / self.imu_rate)
    theta = self.theta_init
    yaw = self.yaw_init

    while ts <= sec2ts(self.time_taken):
      # IMU pose
      rx = self.circle_r * cos(theta)
      ry = self.circle_r * sin(theta)
      rz = 0.0
      r_WS = np.array([rx, ry, rz])
      C_WS = euler321(yaw, 0.0, 0.0)
      T_WS = tf(C_WS, r_WS)

      # IMU velocity
      vx = -self.circle_r * self.w * sin(theta)
      vy = self.circle_r * self.w * cos(theta)
      vz = 0.0
      v_WS = np.array([vx, vy, vz])

      # IMU acceleration
      ax = -self.circle_r * self.w**2 * cos(theta)
      ay = -self.circle_r * self.w**2 * sin(theta)
      az = 0.0
      a_WS = np.array([ax, ay, az])

      # IMU angular velocity
      wx = 0.0
      wy = 0.0
      wz = self.w
      w_WS = np.array([wx, wy, wz])

      # IMU measurements
      acc = C_WS.T @ (a_WS + self.g)
      gyr = C_WS.T @ w_WS

      # Update
      sim_data.timestamps.append(ts)
      sim_data.poses[ts] = T_WS
      sim_data.vel[ts] = v_WS
      sim_data.acc[ts] = acc
      sim_data.gyr[ts] = gyr

      theta += self.w * ts2sec(dt_ns)
      yaw += self.w * ts2sec(dt_ns)
      ts += dt_ns

    return sim_data

  def _sim_cam(self, cam_idx, cam_params, T_BCi):
    """ Simulate camera """
    sim_data = SimCameraData(cam_idx, cam_params, self.features)

    ts = 0
    dt_ns = sec2ts(1.0 / self.cam_rate)
    theta = self.theta_init
    yaw = self.yaw_init

    while ts <= sec2ts(self.time_taken):
      # Body pose
      rx = self.circle_r * cos(theta)
      ry = self.circle_r * sin(theta)
      rz = 0.0
      r_WB = [rx, ry, rz]
      C_WB = euler321(yaw, 0.0, 0.0)
      T_WB = tf(C_WB, r_WB)

      # Simulate camera pose and camera frame
      T_WCi = T_WB @ T_BCi
      cam_frame = SimCameraFrame(ts, cam_idx, cam_params, T_WCi, self.features)
      sim_data.timestamps.append(ts)
      sim_data.poses[ts] = T_WCi
      sim_data.frames[ts] = cam_frame

      # Update
      theta += self.w * ts2sec(dt_ns)
      yaw += self.w * ts2sec(dt_ns)
      ts += dt_ns

    return sim_data

  def _form_timeline(self):
    """ Form timeline """
    # Form timeline
    timeline = Timeline()

    # -- Add imu events
    imu_idx = self.imu0_data.imu_idx
    for ts in self.imu0_data.timestamps:
      acc = self.imu0_data.acc[ts]
      gyr = self.imu0_data.gyr[ts]
      imu_event = ImuEvent(ts, imu_idx, acc, gyr)
      timeline.add_event(ts, imu_event)

    # -- Add camera events
    for cam_idx, cam_data in self.mcam_data.items():
      for ts in cam_data.timestamps:
        frame = cam_data.frames[ts]
        fids = frame.feature_ids
        kps = frame.measurements

        sim_img = []
        for i, fid in enumerate(fids):
          sim_img.append([fid, kps[i]])

        cam_event = CameraEvent(ts, cam_idx, sim_img)
        timeline.add_event(ts, cam_event)

    return timeline


class SimFeatureTracker(FeatureTracker):
  """ Sim Feature Tracker """
  def __init__(self):
    FeatureTracker.__init__(self)

  def update(self, ts, mcam_imgs):
    """ Update Sim Feature Tracker """
    for cam_idx, cam_data in mcam_imgs.items():
      kps = [data[1] for data in cam_data]
      fids = [data[0] for data in cam_data]
      ft_data = FeatureTrackerData(cam_idx, None, kps, fids)
      self.cam_data[cam_idx] = ft_data

    # Update
    self.frame_idx += 1
    self.prev_ts = ts
    self.prev_mcam_imgs = mcam_imgs

    return self.cam_data

  def visualize(self):
    """ Visualize """
    # Image size
    # cam_res = cam0_params.data.resolution
    # img_w, img_h = cam_res
    # img0 = np.zeros((img_h, img_w), dtype=np.uint8)
    # kps = [kp for kp in ft_data[0].keypoints]
    # viz = draw_keypoints(img0, kps)
    # cv2.imshow('viz', viz)
    # cv2.waitKey(0)
    pass


###############################################################################
# CONTROL
###############################################################################


class PID:
  """ PID controller """
  def __init__(self, k_p, k_i, k_d):
    self.k_p = k_p
    self.k_i = k_i
    self.k_d = k_d

    self.error_p = 0.0
    self.error_i = 0.0
    self.error_d = 0.0
    self.error_prev = 0.0
    self.error_sum = 0.0

  def update(self, setpoint, actual, dt):
    """ Update """
    # Calculate errors
    error = setpoint - actual
    self.error_sum += error * dt

    # Calculate output
    self.error_p = self.k_p * error
    self.error_i = self.k_i * self.error_sum
    self.error_d = self.k_d * (error - self.error_prev) / dt
    output = self.error_p + self.error_i + self.error_d

    # Keep track of error
    self.error_prev = error

    return output

  def reset(self):
    """ Reset """


class CarrotController:
  """ Carrot Controller """
  def __init__(self):
    self.waypoints = []
    self.wp_start = None
    self.wp_end = None
    self.wp_index = None
    self.look_ahead_dist = 0.0

  def _calculate_closest_point(self, pos):
    """ Calculate closest point """
    v1 = pos - self.wp_start
    v2 = self.wp_end - self.wp_start
    t = v1 @ v2 / v2.squaredNorm()
    pt = self.wp_start + t * v2

    return (t, pt)

  def _calculate_carrot_point(self, pos):
    """ Calculate carrot point """
    assert len(pos) == 3

    t, closest_pt = self._calculate_closest_point(pos)
    carrot_pt = None

    if t == -1:
      # Closest point is before wp_start
      carrot_pt = self.wp_start

    elif t == 0:
      # Closest point is between wp_start wp_end
      u = self.wp_end - self.wp_start
      v = u / norm(u)
      carrot_pt = closest_pt + self.look_ahead_dist * v

    elif t == 1:
      # Closest point is after wp_end
      carrot_pt = self.wp_end

    return (t, carrot_pt)

  def update(self, pos):
    """ Update """
    assert len(pos) == 3
    # Calculate new carot point
    status, carrot_pt = self._calculate_carrot_point(pos)

    # Check if there are more waypoints
    if (self.wp_index + 1) == len(self.waypoints):
      return None

    # Update waypoints
    if status == 1:
      self.wp_index += 1
      self.wp_start = self.wp_end
      self.wp_end = self.waypoints[self.wp_index]

    return carrot_pt


###############################################################################
# Visualizer
###############################################################################

import websockets
import asyncio

from subprocess import Popen, PIPE


class DevServer:
  """ Dev server """
  def __init__(self, loop_fn):
    self.host = "127.0.0.1"
    self.port = 5000
    self.loop_fn = loop_fn

  def __del__(self):
    process = Popen([f"lsof", "-i", ":{self.port}"], stdout=PIPE, stderr=PIPE)
    stdout, _ = process.communicate()
    for process in str(stdout.decode("utf-8")).split("\n")[1:]:
      data = [x for x in process.split(" ") if x != '']
      if len(data) <= 1:
        continue
      print(f"killing {data[1]}")
      os.kill(int(data[1]), signal.SIGKILL)

  def run(self):
    """ Run server """
    kwargs = {"ping_timeout": 1, "close_timeout": 1}
    server = websockets.serve(self.loop_fn, self.host, self.port, **kwargs)
    loop = asyncio.get_event_loop()
    loop.run_until_complete(server)
    loop.run_forever()

  @staticmethod
  def stop():
    """ Stop server """
    asyncio.get_event_loop().stop()


class MultiPlot:
  """ MultiPlot """
  def __init__(self, has_gnd=False):
    self.plots = []
    self.add_pos_xy_plot(has_gnd=has_gnd)
    self.add_pos_z_plot(has_gnd=has_gnd)
    self.add_roll_plot(has_gnd=has_gnd)
    self.add_pitch_plot(has_gnd=has_gnd)
    self.add_yaw_plot(has_gnd=has_gnd)
    self.add_pos_error_plot()
    self.add_att_error_plot()
    self.add_reproj_error_plot()

    self.plot_data = {}
    self.emit_rate = 8.0  # Hz
    self.last_updated = datetime.now()

  def _add_plot(self, title, xlabel, ylabel, trace_names, **kwargs):
    conf = {}
    conf["title"] = title
    conf["width"] = kwargs.get("width", 300)
    conf["height"] = kwargs.get("height", 280)
    conf["buf_size"] = kwargs.get("buf_size", 100)
    conf["trace_names"] = trace_names
    conf["xlabel"] = xlabel
    conf["ylabel"] = ylabel
    conf["show_legend"] = True if len(trace_names) > 1 else False
    self.plots.append(conf)

  def add_pos_xy_plot(self, **kwargs):
    """ Add Position X-Y Data """
    title = "Position X-Y"
    xlabel = "x [m]"
    ylabel = "y [m]"
    trace_names = ["Estimate"]
    if kwargs.get("has_gnd"):
      trace_names.append("Ground-Truth")

    self._add_plot(title, xlabel, ylabel, trace_names)

  def add_pos_z_plot(self, **kwargs):
    """ Add Position Z Data """
    xlabel = "Time [s]"
    ylabel = "y [m]"
    trace_names = ["Estimate"]
    if kwargs.get("has_gnd"):
      trace_names.append("Ground-Truth")

    self._add_plot("Position Z", xlabel, ylabel, trace_names)

  def add_roll_plot(self, **kwargs):
    """ Add Roll Data """
    xlabel = "Time [s]"
    ylabel = "Attitude [deg]"
    trace_names = ["Estimate"]
    if kwargs.get("has_gnd"):
      trace_names.append("Ground-Truth")

    self._add_plot("Roll", xlabel, ylabel, trace_names)

  def add_pitch_plot(self, **kwargs):
    """ Add Roll Data """
    xlabel = "Time [s]"
    ylabel = "Attitude [deg]"
    trace_names = ["Estimate"]
    if kwargs.get("has_gnd"):
      trace_names.append("Ground-Truth")

    self._add_plot("Pitch", xlabel, ylabel, trace_names)

  def add_yaw_plot(self, **kwargs):
    """ Add Yaw Data """
    xlabel = "Time [s]"
    ylabel = "Attitude [deg]"
    trace_names = ["Estimate"]
    if kwargs.get("has_gnd"):
      trace_names.append("Ground-Truth")

    self._add_plot("Yaw", xlabel, ylabel, trace_names)

  def add_pos_error_plot(self):
    """ Add Position Error Data """
    title = "Position Error"
    xlabel = "Time [s]"
    ylabel = "Position Error [m]"
    trace_names = ["Error"]
    self._add_plot(title, xlabel, ylabel, trace_names)

  def add_att_error_plot(self):
    """ Add Attitude Error Data """
    title = "Attitude Error"
    xlabel = "Time [s]"
    ylabel = "Position Error [m]"
    trace_names = ["Error"]
    self._add_plot(title, xlabel, ylabel, trace_names)

  def add_reproj_error_plot(self):
    """ Add Reprojection Error Data """
    title = "Reprojection Error"
    xlabel = "Time [s]"
    ylabel = "Reprojection Error [px]"
    trace_names = ["Mean", "RMSE"]
    self._add_plot(title, xlabel, ylabel, trace_names)

  def _form_plot_data(self, plot_title, time_s, **kwargs):
    gnd = kwargs.get("gnd")
    est = kwargs.get("est")
    err = kwargs.get("err")

    conf = {plot_title: {}}
    if gnd:
      conf[plot_title]["Ground-Truth"] = {"x": time_s, "y": gnd}

    if est:
      conf[plot_title]["Estimate"] = {"x": time_s, "y": est}

    if err:
      conf[plot_title]["Error"] = {"x": time_s, "y": err}

    self.plot_data.update(conf)

  def add_pos_xy_data(self, **kwargs):
    """ Add Position X-Y Data """
    plot_title = "Position X-Y"
    conf = {plot_title: {}}

    if "gnd" in kwargs:
      gnd = kwargs["gnd"]
      conf[plot_title]["Ground-Truth"] = {"x": gnd[0], "y": gnd[1]}

    if "est" in kwargs:
      est = kwargs["est"]
      conf[plot_title]["Estimate"] = {"x": est[0], "y": est[1]}

    self.plot_data.update(conf)

  def add_pos_z_data(self, time_s, **kwargs):
    """ Add Position Z Data """
    self._form_plot_data("Position Z", time_s, **kwargs)

  def add_roll_data(self, time_s, **kwargs):
    """ Add Roll Data """
    self._form_plot_data("Roll", time_s, **kwargs)

  def add_pitch_data(self, time_s, **kwargs):
    """ Add Roll Data """
    self._form_plot_data("Pitch", time_s, **kwargs)

  def add_yaw_data(self, time_s, **kwargs):
    """ Add Yaw Data """
    self._form_plot_data("Yaw", time_s, **kwargs)

  def add_pos_error_data(self, time_s, error):
    """ Add Position Error Data """
    self._form_plot_data("Position Error", time_s, err=error)

  def add_att_error_data(self, time_s, error):
    """ Add Attitude Error Data """
    self._form_plot_data("Attitude Error", time_s, err=error)

  def add_reproj_error_data(self, time_s, reproj_rmse, reproj_mean):
    """ Add Reprojection Error Data """
    plot_title = "Reprojection Error"
    conf = {plot_title: {}}
    conf[plot_title]["Mean"] = {"x": time_s, "y": reproj_rmse}
    conf[plot_title]["RMSE"] = {"x": time_s, "y": reproj_mean}
    self.plot_data.update(conf)

  def get_plots(self):
    """ Get plots """
    return json.dumps(self.plots)

  def get_plot_data(self):
    """ Get plot data """
    return json.dumps(self.plot_data)

  async def emit_data(self, ws):
    """ Emit data """
    time_now = datetime.now()
    time_diff = (time_now - self.last_updated).total_seconds()
    if time_diff > (1.0 / self.emit_rate):
      await ws.send(self.get_plot_data())
      self.last_updated = time_now


###############################################################################
#                               UNITTESTS
###############################################################################

import unittest

euroc_data_path = '/data/euroc/V1_01'

# NETWORK #####################################################################


def test_websocket_callback():
  """ Test WebSocket Callback """
  time.sleep(1)
  return "Hello World"


class TestNetwork(unittest.TestCase):
  """ Test Network """
  def test_http_parse_request(self):
    """ Test Parsing HTTP Request """
    request_string = """GET / HTTP/1.1\r\n
                        Host: localhost:8080\r\n
                        User-Agent: Mozilla/5.0\r\n
                        Accept-Language: en-GB,en;q=0.5\r\n
                        Accept-Encoding: gzip, deflate\r\n
                        Connection: keep-alive\r\n
                        Upgrade-Insecure-Requests: 1\r\n
                        Sec-Fetch-Dest: document\r\n
                        Sec-Fetch-Mode: navigate\r\n
                        Sec-Fetch-Site: cross-site\r\n
                        Cache-Control: max-age=0\r\n\r\n"""
    (protocol, method, path, headers) = http_parse_request(request_string)
    self.assertTrue(protocol == "HTTP/1.1")
    self.assertTrue(method == "GET")
    self.assertTrue(path == "/")
    self.assertTrue(headers["Host"] == "localhost:8080")
    self.assertTrue(headers["User-Agent"] == "Mozilla/5.0")

  def test_websocket_hash(self):
    """ Test WebSocket Upgrade Response """
    ws_key = "dGhlIHNhbXBsZSBub25jZQ=="
    ws_hash = "s3pPLMBiTxaQ9kYGzzhZRbK+xOo="
    self.assertTrue(websocket_hash(ws_key) == ws_hash)

  def test_websocket_encode_frame(self):
    """ Test WebSocket Frame """
    payload = "Hello World!"
    frame = websocket_encode_frame(payload)
    self.assertTrue(frame is not None)

  # def test_websocket_decode_frame(self):
  #   """ Test WebSocket Frame """
  #   host = '127.0.0.1'
  #   port = 5000
  #   sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  #   sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
  #   sock.bind((host, port))
  #   sock.listen()
  #   conn, _ = sock.accept()
  #
  #   # Request
  #   buf_size = 4096
  #   req_str = conn.recv(buf_size, 0).decode("ascii")
  #   (_, _, _, headers) = http_parse_request(req_str)
  #   ws_key = headers["Sec-WebSocket-Key"]
  #
  #   # Respond
  #   resp = websocket_handshake_response(ws_key)
  #   conn.send(str.encode(resp))
  #
  #   # Decode websocket frame
  #   data = websocket_decode_frame(conn)

  def test_debug_server(self):
    """ Test Debug Server """
    server = DebugServer(test_websocket_callback)
    self.assertTrue(server is not None)


# LINEAR ALGEBRA ##############################################################


class TestLinearAlgebra(unittest.TestCase):
  """ Test Linear Algebra """
  def test_normalize(self):
    """ Test normalize() """
    x = np.array([1.0, 2.0, 3.0])
    x_prime = normalize(x)
    self.assertTrue(isclose(norm(x_prime), 1.0))

  def test_skew(self):
    """ Test skew() """
    x = np.array([1.0, 2.0, 3.0])
    S = np.array([[0.0, -3.0, 2.0], [3.0, 0.0, -1.0], [-2.0, 1.0, 0.0]])
    self.assertTrue(matrix_equal(S, skew(x)))

  def test_skew_inv(self):
    """ Test skew_inv() """
    x = np.array([1.0, 2.0, 3.0])
    S = np.array([[0.0, -3.0, 2.0], [3.0, 0.0, -1.0], [-2.0, 1.0, 0.0]])
    self.assertTrue(matrix_equal(x, skew_inv(S)))

  def test_matrix_equal(self):
    """ Test matrix_equal() """
    A = ones((3, 3))
    B = ones((3, 3))
    self.assertTrue(matrix_equal(A, B))

    C = 2.0 * ones((3, 3))
    self.assertFalse(matrix_equal(A, C))

  # def test_check_jacobian(self):
  #   step_size = 1e-6
  #   threshold = 1e-5
  #
  #   x = 2
  #   y0 = x**2
  #   y1 = (x + step_size)**2
  #   jac = 2 * x
  #   fdiff = y1 - y0
  #
  #   jac_name = "jac"
  #   fdiff = (y1 - y0) / step_size
  #   self.assertTrue(check_jacobian(jac_name, fdiff, jac, threshold))


class TestLie(unittest.TestCase):
  """ Test Lie algebra functions """
  def test_Exp_Log(self):
    """ Test Exp() and Log() """
    pass


# TRANSFORM ###################################################################


class TestTransform(unittest.TestCase):
  """ Test transform functions """
  def test_homogeneous(self):
    """ Test homogeneous() """
    p = np.array([1.0, 2.0, 3.0])
    hp = homogeneous(p)
    self.assertTrue(hp[0] == 1.0)
    self.assertTrue(hp[1] == 2.0)
    self.assertTrue(hp[2] == 3.0)
    self.assertTrue(len(hp) == 4)

  def test_dehomogeneous(self):
    """ Test dehomogeneous() """
    p = np.array([1.0, 2.0, 3.0])
    hp = np.array([1.0, 2.0, 3.0, 1.0])
    p = dehomogeneous(hp)
    self.assertTrue(p[0] == 1.0)
    self.assertTrue(p[1] == 2.0)
    self.assertTrue(p[2] == 3.0)
    self.assertTrue(len(p) == 3)

  def test_rotx(self):
    """ Test rotx() """
    x = np.array([0.0, 1.0, 0.0])
    C = rotx(deg2rad(90.0))
    x_prime = C @ x
    self.assertTrue(np.allclose(x_prime, [0.0, 0.0, 1.0]))

  def test_roty(self):
    """ Test roty() """
    x = np.array([1.0, 0.0, 0.0])
    C = roty(deg2rad(90.0))
    x_prime = C @ x
    self.assertTrue(np.allclose(x_prime, [0.0, 0.0, -1.0]))

  def test_rotz(self):
    """ Test rotz() """
    x = np.array([1.0, 0.0, 0.0])
    C = rotz(deg2rad(90.0))
    x_prime = C @ x
    self.assertTrue(np.allclose(x_prime, [0.0, 1.0, 0.0]))

  def test_aa2quat(self):
    """ Test aa2quat() """
    pass

  def test_rvec2rot(self):
    """ Test rvec2quat() """
    pass

  def test_vecs2axisangle(self):
    """ Test vecs2axisangle() """
    pass

  def test_euler321(self):
    """ Test euler321() """
    C = euler321(0.0, 0.0, 0.0)
    self.assertTrue(np.array_equal(C, eye(3)))

  def test_euler2quat_and_quat2euler(self):
    """ Test euler2quat() and quat2euler() """
    y_in = deg2rad(3.0)
    p_in = deg2rad(2.0)
    r_in = deg2rad(1.0)

    q = euler2quat(y_in, p_in, r_in)
    ypr_out = quat2euler(q)

    self.assertTrue(len(q) == 4)
    self.assertTrue(abs(y_in - ypr_out[0]) < 1e-5)
    self.assertTrue(abs(p_in - ypr_out[1]) < 1e-5)
    self.assertTrue(abs(r_in - ypr_out[2]) < 1e-5)

  def test_quat2rot(self):
    """ Test quat2rot() """
    ypr = np.array([0.1, 0.2, 0.3])
    C_i = euler321(*ypr)
    C_j = quat2rot(euler2quat(*ypr))
    self.assertTrue(np.allclose(C_i, C_j))

  def test_rot2euler(self):
    """ Test rot2euler() """
    ypr = np.array([0.1, 0.2, 0.3])
    C = euler321(*ypr)
    euler = rot2euler(C)
    self.assertTrue(np.allclose(ypr, euler))

  def test_rot2quat(self):
    """ Test rot2quat() """
    ypr = np.array([0.1, 0.2, 0.3])
    C = euler321(*ypr)
    q = rot2quat(C)
    self.assertTrue(np.allclose(quat2euler(q), ypr))

  def test_quat_norm(self):
    """ Test quat_norm() """
    q = np.array([1.0, 0.0, 0.0, 0.0])
    self.assertTrue(isclose(quat_norm(q), 1.0))

  def test_quat_normalize(self):
    """ Test quat_normalize() """
    q = np.array([1.0, 0.1, 0.2, 0.3])
    q = quat_normalize(q)
    self.assertTrue(isclose(quat_norm(q), 1.0))

  def test_quat_conj(self):
    """ Test quat_conj() """
    ypr = np.array([0.1, 0.0, 0.0])
    q = rot2quat(euler321(*ypr))
    q_conj = quat_conj(q)
    self.assertTrue(np.allclose(quat2euler(q_conj), -1.0 * ypr))

  def test_quat_inv(self):
    """ Test quat_inv() """
    ypr = np.array([0.1, 0.0, 0.0])
    q = rot2quat(euler321(*ypr))
    q_inv = quat_inv(q)
    self.assertTrue(np.allclose(quat2euler(q_inv), -1.0 * ypr))

  def test_quat_mul(self):
    """ Test quat_mul() """
    p = euler2quat(deg2rad(3.0), deg2rad(2.0), deg2rad(1.0))
    q = euler2quat(deg2rad(1.0), deg2rad(2.0), deg2rad(3.0))
    r = quat_mul(p, q)
    self.assertTrue(r is not None)

  def test_quat_omega(self):
    """ Test quat_omega() """
    pass

  def test_quat_slerp(self):
    """ Test quat_slerp() """
    q_i = rot2quat(euler321(0.1, 0.0, 0.0))
    q_j = rot2quat(euler321(0.2, 0.0, 0.0))
    q_k = quat_slerp(q_i, q_j, 0.5)
    self.assertTrue(np.allclose(quat2euler(q_k), [0.15, 0.0, 0.0]))

    q_i = rot2quat(euler321(0.0, 0.1, 0.0))
    q_j = rot2quat(euler321(0.0, 0.2, 0.0))
    q_k = quat_slerp(q_i, q_j, 0.5)
    self.assertTrue(np.allclose(quat2euler(q_k), [0.0, 0.15, 0.0]))

    q_i = rot2quat(euler321(0.0, 0.0, 0.1))
    q_j = rot2quat(euler321(0.0, 0.0, 0.2))
    q_k = quat_slerp(q_i, q_j, 0.5)
    self.assertTrue(np.allclose(quat2euler(q_k), [0.0, 0.0, 0.15]))

  def test_tf(self):
    """ Test tf() """
    r = np.array([1.0, 2.0, 3.0])
    q = np.array([0.0, 0.0, 0.0, 1.0])
    T = tf(q, r)

    self.assertTrue(np.allclose(T[0:3, 0:3], quat2rot(q)))
    self.assertTrue(np.allclose(T[0:3, 3], r))


# CV ##########################################################################


class TestCV(unittest.TestCase):
  """ Test computer vision functions """
  def setUp(self):
    # Camera
    img_w = 640
    img_h = 480
    fx = focal_length(img_w, 90.0)
    fy = focal_length(img_w, 90.0)
    cx = img_w / 2.0
    cy = img_h / 2.0
    self.proj_params = [fx, fy, cx, cy]

    # Camera pose in world frame
    C_WC = euler321(-pi / 2, 0.0, -pi / 2)
    r_WC = np.array([0.0, 0.0, 0.0])
    self.T_WC = tf(C_WC, r_WC)

    # 3D World point
    self.p_W = np.array([10.0, 0.0, 0.0])

    # Point w.r.t camera
    self.p_C = tf_point(inv(self.T_WC), self.p_W)
    self.x = np.array([self.p_C[0] / self.p_C[2], self.p_C[1] / self.p_C[2]])

  def test_linear_triangulation(self):
    """ Test linear_triangulation() """
    # Camera i - Camera j extrinsics
    C_CiCj = eye(3)
    r_CiCj = np.array([0.05, 0.0, 0.0])
    T_CiCj = tf(C_CiCj, r_CiCj)

    # Camera 0 pose in world frame
    C_WCi = euler321(-pi / 2, 0.0, -pi / 2)
    r_WCi = np.array([0.0, 0.0, 0.0])
    T_WCi = tf(C_WCi, r_WCi)

    # Camera 1 pose in world frame
    T_WCj = T_WCi @ T_CiCj

    # Projection matrices P_i and P_j
    P_i = pinhole_P(self.proj_params, eye(4))
    P_j = pinhole_P(self.proj_params, T_CiCj)

    # Test multiple times
    nb_tests = 100
    for _ in range(nb_tests):
      # Project feature point p_W to image plane
      x = np.random.uniform(-0.05, 0.05)
      y = np.random.uniform(-0.05, 0.05)
      p_W = np.array([10.0, x, y])
      p_Ci_gnd = tf_point(inv(T_WCi), p_W)
      p_Cj_gnd = tf_point(inv(T_WCj), p_W)
      z_i = pinhole_project(self.proj_params, p_Ci_gnd)
      z_j = pinhole_project(self.proj_params, p_Cj_gnd)

      # Triangulate
      p_Ci_est = linear_triangulation(P_i, P_j, z_i, z_j)
      self.assertTrue(np.allclose(p_Ci_est, p_Ci_gnd))

  def test_pinhole_K(self):
    """ Test pinhole_K() """
    fx = 1.0
    fy = 2.0
    cx = 3.0
    cy = 4.0
    proj_params = [fx, fy, cx, cy]
    K = pinhole_K(proj_params)
    expected = np.array([[1.0, 0.0, 3.0], [0.0, 2.0, 4.0], [0.0, 0.0, 1.0]])

    self.assertTrue(np.array_equal(K, expected))

  def test_pinhole_project(self):
    """ Test pinhole_project() """
    z = pinhole_project(self.proj_params, self.p_C)
    self.assertTrue(isclose(z[0], 320.0))
    self.assertTrue(isclose(z[1], 240.0))

  def test_pinhole_params_jacobian(self):
    """ Test pinhole_params_jacobian() """
    # Pinhole params jacobian
    fx, fy, cx, cy = self.proj_params
    z = np.array([fx * self.x[0] + cx, fy * self.x[1] + cy])
    J = pinhole_params_jacobian(self.x)

    # Perform numerical diff to obtain finite difference
    step_size = 1e-6
    tol = 1e-4
    finite_diff = zeros((2, 4))

    for i in range(4):
      params_diff = list(self.proj_params)
      params_diff[i] += step_size
      fx, fy, cx, cy = params_diff

      z_diff = np.array([fx * self.x[0] + cx, fy * self.x[1] + cy])
      finite_diff[0:2, i] = (z_diff - z) / step_size

    self.assertTrue(matrix_equal(finite_diff, J, tol, True))

  def test_pinhole_point_jacobian(self):
    """ Test pinhole_point_jacobian() """
    # Pinhole params jacobian
    fx, fy, cx, cy = self.proj_params
    z = np.array([fx * self.x[0] + cx, fy * self.x[1] + cy])
    J = pinhole_point_jacobian(self.proj_params)

    # Perform numerical diff to obtain finite difference
    step_size = 1e-6
    tol = 1e-4
    finite_diff = zeros((2, 2))

    for i in range(2):
      x_diff = list(self.x)
      x_diff[i] += step_size

      z_diff = np.array([fx * x_diff[0] + cx, fy * x_diff[1] + cy])
      finite_diff[0:2, i] = (z_diff - z) / step_size

    self.assertTrue(matrix_equal(finite_diff, J, tol, True))


# DATASET  ####################################################################


class TestEuroc(unittest.TestCase):
  """ Test Euroc dataset loader """
  def test_load(self):
    """ Test load """
    dataset = EurocDataset(euroc_data_path)
    self.assertTrue(dataset is not None)


class TestKitti(unittest.TestCase):
  """ Test KITTI dataset loader """
  @unittest.skip("")
  def test_load(self):
    """ Test load """
    data_dir = '/data/kitti'
    date = "2011_09_26"
    seq = "93"
    dataset = KittiRawDataset(data_dir, date, seq, True)
    # dataset.plot_frames()

    for i in range(dataset.nb_camera_images()):
      cam0_img = dataset.get_camera_image(0, index=i)
      cam1_img = dataset.get_camera_image(1, index=i)
      cam2_img = dataset.get_camera_image(2, index=i)
      cam3_img = dataset.get_camera_image(3, index=i)

      img_size = cam0_img.shape
      img_new_size = (int(img_size[1] / 2.0), int(img_size[0] / 2.0))

      cam0_img = cv2.resize(cam0_img, img_new_size)
      cam1_img = cv2.resize(cam1_img, img_new_size)
      cam2_img = cv2.resize(cam2_img, img_new_size)
      cam3_img = cv2.resize(cam3_img, img_new_size)

      cv2.imshow("viz", cv2.vconcat([cam0_img, cam1_img, cam2_img, cam3_img]))
      cv2.waitKey(0)

    self.assertTrue(dataset is not None)


# STATE ESTIMATION ############################################################


class TestFactors(unittest.TestCase):
  """ Test factors """
  def test_pose_factor(self):
    """ Test pose factor """
    # Setup camera pose T_WC
    rot = euler2quat(-pi / 2.0, 0.0, -pi / 2.0)
    trans = np.array([0.1, 0.2, 0.3])
    T_WC = tf(rot, trans)

    rot = euler2quat(-pi / 2.0 + 0.01, 0.0 + 0.01, -pi / 2.0 + 0.01)
    trans = np.array([0.1 + 0.01, 0.2 + 0.01, 0.3 + 0.01])
    T_WC_diff = tf(rot, trans)
    pose_est = pose_setup(0, T_WC_diff)

    # Create factor
    param_ids = [0]
    covar = eye(6)
    factor = PoseFactor(param_ids, T_WC, covar)

    # Test jacobians
    fvars = [pose_est]
    self.assertTrue(factor.check_jacobian(fvars, 0, "J_pose"))

  def test_ba_factor(self):
    """ Test ba factor """
    # Setup camera pose T_WC
    rot = euler2quat(-pi / 2.0, 0.0, -pi / 2.0)
    trans = np.array([0.1, 0.2, 0.3])
    T_WC = tf(rot, trans)
    cam_pose = pose_setup(0, T_WC)

    # Setup cam0
    cam_idx = 0
    img_w = 640
    img_h = 480
    res = [img_w, img_h]
    fov = 60.0
    fx = focal_length(img_w, fov)
    fy = focal_length(img_h, fov)
    cx = img_w / 2.0
    cy = img_h / 2.0
    params = [fx, fy, cx, cy, -0.01, 0.01, 1e-4, 1e-4]
    cam_params = camera_params_setup(cam_idx, res, "pinhole", "radtan4", params)
    cam_geom = camera_geometry_setup(cam_idx, res, "pinhole", "radtan4")

    # Setup feature
    p_W = np.array([10, random.uniform(0.0, 1.0), random.uniform(0.0, 1.0)])
    # -- Feature XYZ parameterization
    feature = feature_setup(p_W)
    # # -- Feature inverse depth parameterization
    # param = idp_param(camera, T_WC, z)
    # feature = feature_init(0, param)
    # -- Calculate image point
    p_C = tf_point(inv(T_WC), p_W)
    status, z = cam_geom.project(cam_params.param, p_C)
    self.assertTrue(status)

    # Setup factor
    param_ids = [0, 1, 2]
    factor = BAFactor(cam_geom, param_ids, z)

    # Test jacobians
    fvars = [cam_pose, feature, cam_params]
    self.assertTrue(factor.check_jacobian(fvars, 0, "J_cam_pose"))
    self.assertTrue(factor.check_jacobian(fvars, 1, "J_feature"))
    self.assertTrue(factor.check_jacobian(fvars, 2, "J_cam_params"))

  def test_vision_factor(self):
    """ Test vision factor """
    # Setup camera pose T_WB
    rot = euler2quat(0.01, 0.01, 0.03)
    trans = np.array([0.001, 0.002, 0.003])
    T_WB = tf(rot, trans)
    pose = pose_setup(0, T_WB)

    # Setup camera extrinsics T_BCi
    rot = euler2quat(-pi / 2.0, 0.0, -pi / 2.0)
    trans = np.array([0.1, 0.2, 0.3])
    T_BCi = tf(rot, trans)
    cam_exts = extrinsics_setup(T_BCi)

    # Setup cam0
    cam_idx = 0
    img_w = 640
    img_h = 480
    res = [img_w, img_h]
    fov = 60.0
    fx = focal_length(img_w, fov)
    fy = focal_length(img_h, fov)
    cx = img_w / 2.0
    cy = img_h / 2.0
    params = [fx, fy, cx, cy, -0.01, 0.01, 1e-4, 1e-4]
    cam_params = camera_params_setup(cam_idx, res, "pinhole", "radtan4", params)
    cam_geom = camera_geometry_setup(cam_idx, res, "pinhole", "radtan4")

    # Setup feature
    p_W = np.array([10, random.uniform(0.0, 1.0), random.uniform(0.0, 1.0)])
    # -- Feature XYZ parameterization
    feature = feature_setup(p_W)
    # # -- Feature inverse depth parameterization
    # param = idp_param(camera, T_WC, z)
    # feature = feature_init(0, param)
    # -- Calculate image point
    T_WCi = T_WB * T_BCi
    p_C = tf_point(inv(T_WCi), p_W)
    status, z = cam_geom.project(cam_params.param, p_C)
    self.assertTrue(status)

    # Setup factor
    param_ids = [0, 1, 2, 3]
    factor = VisionFactor(cam_geom, param_ids, z)

    # Test jacobians
    fvars = [pose, cam_exts, feature, cam_params]
    self.assertTrue(factor.check_jacobian(fvars, 0, "J_pose"))
    self.assertTrue(factor.check_jacobian(fvars, 1, "J_cam_exts"))
    self.assertTrue(factor.check_jacobian(fvars, 2, "J_feature"))
    self.assertTrue(factor.check_jacobian(fvars, 3, "J_cam_params"))

  def test_calib_vision_factor(self):
    """ Test CalibVisionFactor """
    # Calibration target pose T_WF
    C_WF = euler321(-pi / 2.0, 0.0, deg2rad(80.0))
    r_WF = np.array([0.001, 0.001, 0.001])
    T_WF = tf(C_WF, r_WF)

    # Body pose T_WB
    rot = euler2quat(-pi / 2.0, 0.0, -pi / 2.0)
    trans = np.array([-10.0, 0.0, 0.0])
    T_WB = tf(rot, trans)

    # Relative pose T_BF
    T_BF = inv(T_WB) @ T_WF

    # Camera extrinsics T_BCi
    rot = eye(3)
    trans = np.array([0.001, 0.002, 0.003])
    T_BCi = tf(rot, trans)

    # Camera 0
    cam_idx = 0
    img_w = 640
    img_h = 480
    res = [img_w, img_h]
    fov = 90.0
    fx = focal_length(img_w, fov)
    fy = focal_length(img_h, fov)
    cx = img_w / 2.0
    cy = img_h / 2.0
    params = [fx, fy, cx, cy, -0.01, 0.01, 1e-4, 1e-4]
    cam_params = camera_params_setup(cam_idx, res, "pinhole", "radtan4", params)
    cam_geom = camera_geometry_setup(cam_idx, res, "pinhole", "radtan4")

    # Test factor
    grid = AprilGrid()
    tag_id = 1
    corner_idx = 2
    r_FFi = grid.get_object_point(tag_id, corner_idx)
    T_CiF = inv(T_BCi) @ T_BF
    r_CiFi = tf_point(T_CiF, r_FFi)
    status, z = cam_geom.project(cam_params.param, r_CiFi)
    self.assertTrue(status)

    pids = [0, 1, 2]
    grid_data = (tag_id, corner_idx, r_FFi, z)
    factor = CalibVisionFactor(cam_geom, pids, grid_data)

    # Test jacobianstf(rot, trans)
    rel_pose = pose_setup(0, T_BF)
    cam_exts = extrinsics_setup(T_BCi)
    fvars = [rel_pose, cam_exts, cam_params]
    self.assertTrue(factor.check_jacobian(fvars, 0, "J_rel_pose"))
    self.assertTrue(factor.check_jacobian(fvars, 1, "J_cam_exts"))
    self.assertTrue(factor.check_jacobian(fvars, 2, "J_cam_params"))

  def test_imu_buffer(self):
    """ Test IMU Buffer """
    # Extract measurements from ts: 4 - 7
    imu_buf = ImuBuffer()
    for k in range(10):
      ts = k
      acc = np.array([0.0 + k, 0.0 + k, 0.0 + k])
      gyr = np.array([0.0 + k, 0.0 + k, 0.0 + k])
      imu_buf.add(ts, acc, gyr)

    # print("Original imu_buf:")
    # imu_buf.print(True)
    imu_buf2 = imu_buf.extract(4, 7)
    # print("Extracted imu_buf2 (ts: 4 - 7):")
    # imu_buf2.print(True)
    # print("Modified imu_buf:")
    # imu_buf.print(True)

    self.assertTrue(imu_buf.length() == 4)
    self.assertTrue(imu_buf.ts[0] == 6)
    self.assertTrue(imu_buf.ts[-1] == 9)

    self.assertTrue(imu_buf2.length() == 4)
    self.assertTrue(imu_buf2.ts[0] == 4)
    self.assertTrue(imu_buf2.ts[-1] == 7)

  def test_imu_buffer_with_interpolation(self):
    """ Test IMU Buffer with interpolation """
    # Interpolation test
    imu_buf = ImuBuffer()
    for k in range(10):
      ts = k
      acc = np.array([0.0 + k, 0.0 + k, 0.0 + k])
      gyr = np.array([0.0 + k, 0.0 + k, 0.0 + k])
      imu_buf.add(ts, acc, gyr)

    # print("Original imu_buf:")
    # imu_buf.print(True)
    imu_buf2 = imu_buf.extract(4.25, 8.9)
    # print("Extracted imu_buf2 (ts: 4.25 - 8.9):")
    # imu_buf2.print(True)
    # print("Modified imu_buf:")
    # imu_buf.print(True)

    self.assertTrue(imu_buf.length() == 2)
    self.assertTrue(imu_buf.ts[0] == 8)
    self.assertTrue(imu_buf.ts[-1] == 9)

    self.assertTrue(imu_buf2.length() == 6)
    self.assertTrue(imu_buf2.ts[0] == 4.25)
    self.assertTrue(imu_buf2.ts[-1] == 8.9)

  def test_imu_factor_propagate(self):
    """ Test IMU factor propagate """
    # Sim imu data
    circle_r = 0.5
    circle_v = 1.0
    sim_data = SimData(circle_r, circle_v, sim_cams=False)
    imu_data = sim_data.imu0_data

    # Setup imu parameters
    noise_acc = 0.08  # accelerometer measurement noise stddev.
    noise_gyr = 0.004  # gyroscope measurement noise stddev.
    noise_ba = 0.00004  # accelerometer bias random work noise stddev.
    noise_bg = 2.0e-6  # gyroscope bias random work noise stddev.
    imu_params = ImuParams(noise_acc, noise_gyr, noise_ba, noise_bg)

    # Setup imu buffer
    start_idx = 0
    end_idx = 10
    # end_idx = len(imu_data.timestamps) - 1
    imu_buf = imu_data.form_imu_buffer(start_idx, end_idx)

    # Pose i
    ts_i = imu_buf.ts[start_idx]
    T_WS_i = imu_data.poses[ts_i]

    # Speed and bias i
    ts_i = imu_buf.ts[start_idx]
    vel_i = imu_data.vel[ts_i]
    ba_i = np.array([0.0, 0.0, 0.0])
    bg_i = np.array([0.0, 0.0, 0.0])
    sb_i = speed_biases_setup(ts_i, vel_i, bg_i, ba_i)

    # Propagate imu measurements
    data = ImuFactor.propagate(imu_buf, imu_params, sb_i)

    # Check propagation
    ts_j = imu_data.timestamps[end_idx - 1]
    T_WS_j_est = T_WS_i @ tf(data.dC, data.dr)
    C_WS_j_est = tf_rot(T_WS_j_est)
    T_WS_j_gnd = imu_data.poses[ts_j]
    C_WS_j_gnd = tf_rot(T_WS_j_gnd)
    # -- Position
    trans_diff = norm(tf_trans(T_WS_j_gnd) - tf_trans(T_WS_j_est))
    self.assertTrue(trans_diff < 0.05)
    # -- Rotation
    dC = C_WS_j_gnd.T * C_WS_j_est
    dq = quat_normalize(rot2quat(dC))
    dC = quat2rot(dq)
    rpy_diff = rad2deg(acos((trace(dC) - 1.0) / 2.0))
    self.assertTrue(rpy_diff < 1.0)

  def test_imu_factor(self):
    """ Test IMU factor """
    # Simulate imu data
    circle_r = 0.5
    circle_v = 1.0
    sim_data = SimData(circle_r, circle_v, sim_cams=False)
    imu_data = sim_data.imu0_data

    # Setup imu parameters
    noise_acc = 0.08  # accelerometer measurement noise stddev.
    noise_gyr = 0.004  # gyroscope measurement noise stddev.
    noise_ba = 0.00004  # accelerometer bias random work noise stddev.
    noise_bg = 2.0e-6  # gyroscope bias random work noise stddev.
    imu_params = ImuParams(noise_acc, noise_gyr, noise_ba, noise_bg)

    # Setup imu buffer
    start_idx = 0
    end_idx = 10
    imu_buf = imu_data.form_imu_buffer(start_idx, end_idx)

    # Pose i
    ts_i = imu_buf.ts[start_idx]
    T_WS_i = imu_data.poses[ts_i]
    pose_i = pose_setup(ts_i, T_WS_i)

    # Pose j
    ts_j = imu_buf.ts[end_idx - 1]
    T_WS_j = imu_data.poses[ts_j]
    pose_j = pose_setup(ts_j, T_WS_j)

    # Speed and bias i
    vel_i = imu_data.vel[ts_i]
    ba_i = np.array([0.0, 0.0, 0.0])
    bg_i = np.array([0.0, 0.0, 0.0])
    sb_i = speed_biases_setup(ts_i, vel_i, ba_i, bg_i)

    # Speed and bias j
    vel_j = imu_data.vel[ts_j]
    ba_j = np.array([0.0, 0.0, 0.0])
    bg_j = np.array([0.0, 0.0, 0.0])
    sb_j = speed_biases_setup(ts_j, vel_j, ba_j, bg_j)

    # Setup IMU factor
    param_ids = [0, 1, 2, 3]
    factor = ImuFactor(param_ids, imu_params, imu_buf, sb_i)

    # Test jacobians
    # yapf: disable
    fvars = [pose_i, sb_i, pose_j, sb_j]
    self.assertTrue(factor)
    self.assertTrue(factor.check_jacobian(fvars, 0, "J_pose_i", threshold=1e-3))
    self.assertTrue(factor.check_jacobian(fvars, 1, "J_sb_i"))
    self.assertTrue(factor.check_jacobian(fvars, 2, "J_pose_j", threshold=1e-3))
    self.assertTrue(factor.check_jacobian(fvars, 3, "J_sb_j"))
    # yapf: enable


class TestFactorGraph(unittest.TestCase):
  """ Test Factor Graph """
  @classmethod
  def setUpClass(cls):
    super(TestFactorGraph, cls).setUpClass()
    circle_r = 5.0
    circle_v = 1.0
    pickle_path = '/tmp/sim_data.pickle'
    cls.sim_data = SimData.create_or_load(circle_r, circle_v, pickle_path)

  def setUp(self):
    self.sim_data = TestFactorGraph.sim_data

  def test_factor_graph_add_param(self):
    """ Test FactorGrpah.add_param() """
    # Setup camera pose T_WC
    rot = euler2quat(-pi / 2.0, 0.0, -pi / 2.0)
    trans = np.array([0.1, 0.2, 0.3])
    T_WC = tf(rot, trans)
    pose0 = pose_setup(0, T_WC)
    pose1 = pose_setup(1, T_WC)

    # Add params
    graph = FactorGraph()
    pose0_id = graph.add_param(pose0)
    pose1_id = graph.add_param(pose1)

    # Assert
    self.assertEqual(pose0_id, 0)
    self.assertEqual(pose1_id, 1)
    self.assertNotEqual(pose0, pose1)
    self.assertEqual(graph.params[pose0_id], pose0)
    self.assertEqual(graph.params[pose1_id], pose1)

  def test_factor_graph_add_factor(self):
    """ Test FactorGrpah.add_factor() """
    # Setup factor graph
    graph = FactorGraph()

    # Setup camera pose T_WC
    rot = euler2quat(-pi / 2.0, 0.0, -pi / 2.0)
    trans = np.array([0.1, 0.2, 0.3])
    T_WC = tf(rot, trans)
    pose = pose_setup(0, T_WC)
    pose_id = graph.add_param(pose)

    # Create factor
    param_ids = [pose_id]
    covar = eye(6)
    pose_factor = PoseFactor(param_ids, T_WC, covar)
    pose_factor_id = graph.add_factor(pose_factor)

    # Assert
    self.assertEqual(len(graph.params), 1)
    self.assertEqual(len(graph.factors), 1)
    self.assertEqual(graph.factors[pose_factor_id], pose_factor)

  def test_factor_graph_solve_vo(self):
    """ Test solving a visual odometry problem """
    # Sim data
    cam0_data = self.sim_data.get_camera_data(0)
    cam0_params = self.sim_data.get_camera_params(0)
    cam0_geom = self.sim_data.get_camera_geometry(0)

    # Setup factor graph
    poses_gnd = []
    poses_init = []
    poses_est = []
    graph = FactorGraph()

    # -- Add features
    features = self.sim_data.features
    feature_ids = []
    for i in range(features.shape[0]):
      p_W = features[i, :]
      # p_W += np.random.rand(3) * 0.1  # perturb feature
      feature = feature_setup(p_W, fix=True)
      feature_ids.append(graph.add_param(feature))

    # -- Add cam0
    cam0_id = graph.add_param(cam0_params)

    # -- Build bundle adjustment problem
    nb_poses = 0
    for ts in cam0_data.timestamps:
      # Camera frame at ts
      cam_frame = cam0_data.frames[ts]

      # Add camera pose T_WC0
      T_WC0_gnd = cam0_data.poses[ts]
      # -- Perturb camera pose
      trans_rand = np.random.rand(3)
      rvec_rand = np.random.rand(3) * 0.1
      T_WC0_init = tf_update(T_WC0_gnd, np.block([*trans_rand, *rvec_rand]))
      # -- Add to graph
      pose = pose_setup(ts, T_WC0_init)
      pose_id = graph.add_param(pose)
      poses_gnd.append(T_WC0_gnd)
      poses_init.append(T_WC0_init)
      poses_est.append(pose_id)
      nb_poses += 1

      # Add ba factors
      for i, idx in enumerate(cam_frame.feature_ids):
        z = cam_frame.measurements[i]
        param_ids = [pose_id, feature_ids[idx], cam0_id]
        graph.add_factor(BAFactor(cam0_geom, param_ids, z))

    # Solve
    # debug = True
    debug = False
    # prof = profile_start()
    graph.solve(debug)
    # profile_stop(prof)

    # Visualize
    if debug:
      pos_gnd = np.array([tf_trans(T) for T in poses_gnd])
      pos_init = np.array([tf_trans(T) for T in poses_init])
      pos_est = []
      for pose_pid in poses_est:
        pose = graph.params[pose_pid]
        pos_est.append(tf_trans(pose2tf(pose.param)))
      pos_est = np.array(pos_est)

      plt.figure()
      plt.plot(pos_gnd[:, 0], pos_gnd[:, 1], 'g-', label="Ground Truth")
      plt.plot(pos_init[:, 0], pos_init[:, 1], 'r-', label="Initial")
      plt.plot(pos_est[:, 0], pos_est[:, 1], 'b-', label="Estimated")
      plt.xlabel("Displacement [m]")
      plt.ylabel("Displacement [m]")
      plt.legend(loc=0)
      plt.show()

    # Asserts
    errors = graph.get_reproj_errors()
    self.assertTrue(rmse(errors) < 0.1)

  def test_factor_graph_solve_io(self):
    """ Test solving a pure inertial odometry problem """
    # Imu params
    noise_acc = 0.08  # accelerometer measurement noise stddev.
    noise_gyr = 0.004  # gyroscope measurement noise stddev.
    noise_ba = 0.00004  # accelerometer bias random work noise stddev.
    noise_bg = 2.0e-6  # gyroscope bias random work noise stddev.
    imu_params = ImuParams(noise_acc, noise_gyr, noise_ba, noise_bg)

    # Setup factor graph
    imu0_data = self.sim_data.imu0_data
    window_size = 20
    start_idx = 0
    # end_idx = 200
    # end_idx = 2000
    # end_idx = int((len(imu0_data.timestamps) - 1) / 2.0)
    end_idx = len(imu0_data.timestamps)

    poses_init = []
    poses_est = []
    sb_est = []
    graph = FactorGraph()

    # -- Pose i
    ts_i = imu0_data.timestamps[start_idx]
    T_WS_i = imu0_data.poses[ts_i]
    pose_i = pose_setup(ts_i, T_WS_i)
    pose_i_id = graph.add_param(pose_i)
    poses_init.append(T_WS_i)
    poses_est.append(pose_i_id)

    # -- Speed and biases i
    vel_i = imu0_data.vel[ts_i]
    ba_i = np.array([0.0, 0.0, 0.0])
    bg_i = np.array([0.0, 0.0, 0.0])
    sb_i = speed_biases_setup(ts_i, vel_i, ba_i, bg_i)
    sb_i_id = graph.add_param(sb_i)
    sb_est.append(sb_i_id)

    for ts_idx in range(start_idx + window_size, end_idx, window_size):
      # -- Pose j
      ts_j = imu0_data.timestamps[ts_idx]
      T_WS_j = imu0_data.poses[ts_j]
      # ---- Pertrub pose j
      trans_rand = np.random.rand(3)
      rvec_rand = np.random.rand(3) * 0.01
      T_WS_j = tf_update(T_WS_j, np.block([*trans_rand, *rvec_rand]))
      # ---- Add to factor graph
      pose_j = pose_setup(ts_j, T_WS_j)
      pose_j_id = graph.add_param(pose_j)

      # -- Speed and biases j
      vel_j = imu0_data.vel[ts_j]
      ba_j = np.array([0.0, 0.0, 0.0])
      bg_j = np.array([0.0, 0.0, 0.0])
      sb_j = speed_biases_setup(ts_j, vel_j, ba_j, bg_j)
      sb_j_id = graph.add_param(sb_j)

      # ---- Keep track of initial and estimate pose
      poses_init.append(T_WS_j)
      poses_est.append(pose_j_id)
      sb_est.append(sb_j_id)

      # -- Imu Factor
      param_ids = [pose_i_id, sb_i_id, pose_j_id, sb_j_id]
      imu_buf = imu0_data.form_imu_buffer(ts_idx - window_size, ts_idx)
      factor = ImuFactor(param_ids, imu_params, imu_buf, sb_i)
      graph.add_factor(factor)

      # -- Update
      pose_i_id = pose_j_id
      pose_i = pose_j
      sb_i_id = sb_j_id
      sb_i = sb_j

    # Solve
    # debug = False
    debug = True
    # prof = profile_start()
    graph.solve(debug)
    # profile_stop(prof)

    if debug:
      pos_init = np.array([tf_trans(T) for T in poses_init])

      pos_est = []
      for pose_pid in poses_est:
        pose = graph.params[pose_pid]
        pos_est.append(tf_trans(pose2tf(pose.param)))
      pos_est = np.array(pos_est)

      sb_est = [graph.params[pid] for pid in sb_est]
      sb_ts0 = sb_est[0].ts
      sb_time = np.array([ts2sec(sb.ts - sb_ts0) for sb in sb_est])
      vel_est = np.array([sb.param[0:3] for sb in sb_est])
      ba_est = np.array([sb.param[3:6] for sb in sb_est])
      bg_est = np.array([sb.param[6:9] for sb in sb_est])

      plt.figure()
      plt.subplot(411)
      plt.plot(pos_init[:, 0], pos_init[:, 1], 'r-')
      plt.plot(pos_est[:, 0], pos_est[:, 1], 'b-')
      plt.xlabel("Displacement [m]")
      plt.ylabel("Displacement [m]")

      plt.subplot(412)
      plt.plot(sb_time, vel_est[:, 0], 'r-')
      plt.plot(sb_time, vel_est[:, 1], 'g-')
      plt.plot(sb_time, vel_est[:, 2], 'b-')
      plt.xlabel("Time [s]")
      plt.ylabel("Velocity [ms^-1]")

      plt.subplot(413)
      plt.plot(sb_time, ba_est[:, 0], 'r-')
      plt.plot(sb_time, ba_est[:, 1], 'g-')
      plt.plot(sb_time, ba_est[:, 2], 'b-')
      plt.xlabel("Time [s]")
      plt.ylabel("Accelerometer Bias [m s^-2]")

      plt.subplot(414)
      plt.plot(sb_time, bg_est[:, 0], 'r-')
      plt.plot(sb_time, bg_est[:, 1], 'g-')
      plt.plot(sb_time, bg_est[:, 2], 'b-')
      plt.xlabel("Time [s]")
      plt.ylabel("Gyroscope Bias [rad s^-1]")

      plt.show()

  # @unittest.skip("")
  def test_factor_graph_solve_vio(self):
    """ Test solving a visual inertial odometry problem """
    # Imu params
    noise_acc = 0.08  # accelerometer measurement noise stddev.
    noise_gyr = 0.004  # gyroscope measurement noise stddev.
    noise_ba = 0.00004  # accelerometer bias random work noise stddev.
    noise_bg = 2.0e-6  # gyroscope bias random work noise stddev.
    imu_params = ImuParams(noise_acc, noise_gyr, noise_ba, noise_bg)

    # Sim data
    cam_idx = 0
    cam_data = self.sim_data.get_camera_data(cam_idx)
    cam_params = self.sim_data.get_camera_params(cam_idx)
    cam_geom = self.sim_data.get_camera_geometry(cam_idx)
    cam_exts = self.sim_data.get_camera_extrinsics(cam_idx)
    cam_params.fix = True
    cam_exts.fix = True

    # Setup factor graph
    poses_gnd = []
    poses_init = []
    poses_est = []
    graph = FactorGraph()
    graph.solver_lambda = 1e4
    graph.solver_max_iter = 10

    # -- Add features
    features = self.sim_data.features
    feature_ids = []
    for i in range(features.shape[0]):
      p_W = features[i, :]
      p_W += np.random.rand(3) * 0.1  # perturb feature
      feature = feature_setup(p_W, fix=False)
      feature_ids.append(graph.add_param(feature))

    # -- Add cam
    cam_id = graph.add_param(cam_params)
    exts_id = graph.add_param(cam_exts)
    T_BC_gnd = pose2tf(cam_exts.param)
    T_CB_gnd = inv(T_BC_gnd)

    # -- Build bundle adjustment problem
    imu_data = ImuBuffer()
    poses = []
    sbs = []

    for ts_k in self.sim_data.timeline.get_timestamps():
      for event in self.sim_data.timeline.get_events(ts_k):
        if isinstance(event, ImuEvent):
          imu_data.add_event(event)

        elif isinstance(event, CameraEvent) and event.cam_idx == cam_idx:
          if imu_data.length() == 0:
            continue

          # Vision factors
          # -- Add camera pose T_WC
          T_WC_gnd = cam_data.poses[ts_k]
          T_WB_gnd = T_WC_gnd @ T_CB_gnd
          # ---- Perturb camera pose
          trans_rand = np.random.rand(3)
          rvec_rand = np.random.rand(3) * 0.1
          T_perturb = np.block([*trans_rand, *rvec_rand])
          T_WB_init = tf_update(T_WB_gnd, T_perturb)
          # ---- Add to graph
          pose = pose_setup(ts_k, T_WB_init)
          poses.append(pose)
          pose_id = graph.add_param(pose)
          poses_gnd.append(T_WB_gnd)
          poses_init.append(T_WB_init)
          poses_est.append(pose_id)
          # -- Speed and biases
          vel_j = self.sim_data.imu0_data.vel[ts_k]
          ba_j = np.array([0.0, 0.0, 0.0])
          bg_j = np.array([0.0, 0.0, 0.0])
          sb = speed_biases_setup(ts_k, vel_j, bg_j, ba_j)
          graph.add_param(sb)
          sbs.append(sb)
          # -- Add vision factors
          for i, idx in enumerate(cam_data.frames[ts_k].feature_ids):
            z = cam_data.frames[ts_k].measurements[i]
            param_ids = [pose_id, exts_id, feature_ids[idx], cam_id]
            graph.add_factor(VisionFactor(cam_geom, param_ids, z))

          # Imu factor
          if len(poses) >= 2:
            ts_km1 = poses[-2].ts
            pose_i_id = poses[-2].param_id
            pose_j_id = poses[-1].param_id
            sb_i_id = sbs[-2].param_id
            sb_j_id = sbs[-1].param_id
            param_ids = [pose_i_id, sb_i_id, pose_j_id, sb_j_id]

            # print(f"ts_k: {ts_k}")
            # print(f"imu.ts[-1]: {imu_data.ts[-1]}")
            if ts_k <= imu_data.ts[-1]:
              imu_buf = imu_data.extract(ts_km1, ts_k)
              graph.add_factor(
                  ImuFactor(param_ids, imu_params, imu_buf, sbs[-2]))

    # Solve
    # debug = True
    debug = False
    # prof = profile_start()
    graph.solve(debug)
    # profile_stop(prof)

    # Visualize
    if debug:
      pos_gnd = np.array([tf_trans(T) for T in poses_gnd])
      pos_init = np.array([tf_trans(T) for T in poses_init])
      pos_est = []
      for pose_pid in poses_est:
        pose = graph.params[pose_pid]
        pos_est.append(tf_trans(pose2tf(pose.param)))
      pos_est = np.array(pos_est)

      plt.figure()
      plt.plot(pos_gnd[:, 0], pos_gnd[:, 1], 'g-', label="Ground Truth")
      plt.plot(pos_init[:, 0], pos_init[:, 1], 'r-', label="Initial")
      plt.plot(pos_est[:, 0], pos_est[:, 1], 'b-', label="Estimated")
      plt.xlabel("Displacement [m]")
      plt.ylabel("Displacement [m]")
      plt.legend(loc=0)
      plt.show()

    # Asserts
    errors = graph.get_reproj_errors()
    self.assertTrue(rmse(errors) < 0.1)


class TestFeatureTracking(unittest.TestCase):
  """ Test feature tracking functions """
  @classmethod
  def setUpClass(cls):
    super(TestFeatureTracking, cls).setUpClass()
    cls.dataset = EurocDataset(euroc_data_path)

  def setUp(self):
    # Setup test images
    self.dataset = TestFeatureTracking.dataset
    ts = self.dataset.cam0_data.timestamps[800]
    img0_path = self.dataset.cam0_data.image_paths[ts]
    img1_path = self.dataset.cam1_data.image_paths[ts]
    self.img0 = cv2.imread(img0_path, cv2.IMREAD_GRAYSCALE)
    self.img1 = cv2.imread(img1_path, cv2.IMREAD_GRAYSCALE)

  def test_spread_keypoints(self):
    """ Test spread_keypoints() """
    # img = np.zeros((140, 160))
    # kps = []
    # kps.append(cv2.KeyPoint(10, 10, 0, 0.0, 0.0, 0))
    # kps.append(cv2.KeyPoint(150, 130, 0, 0.0, 0.0, 1))
    # kps = spread_keypoints(img, kps, 5, debug=True)

    detector = cv2.FastFeatureDetector_create(threshold=50)
    kwargs = {'optflow_mode': True, 'debug': False}
    kps = grid_detect(detector, self.img0, **kwargs)
    kps = spread_keypoints(self.img0, kps, 20, debug=False)

    self.assertTrue(len(kps))

  def test_feature_grid_cell_index(self):
    """ Test FeatureGrid.grid_cell_index() """
    grid_rows = 4
    grid_cols = 4
    image_shape = (280, 320)
    keypoints = [[0, 0], [320, 0], [0, 280], [320, 280]]
    grid = FeatureGrid(grid_rows, grid_cols, image_shape, keypoints)

    self.assertEqual(grid.cell[0], 1)
    self.assertEqual(grid.cell[3], 1)
    self.assertEqual(grid.cell[12], 1)
    self.assertEqual(grid.cell[15], 1)

  def test_feature_grid_count(self):
    """ Test FeatureGrid.count() """
    grid_rows = 4
    grid_cols = 4
    image_shape = (280, 320)
    pts = [[0, 0], [320, 0], [0, 280], [320, 280]]
    grid = FeatureGrid(grid_rows, grid_cols, image_shape, pts)

    self.assertEqual(grid.count(0), 1)
    self.assertEqual(grid.count(3), 1)
    self.assertEqual(grid.count(12), 1)
    self.assertEqual(grid.count(15), 1)

  def test_grid_detect(self):
    """ Test grid_detect() """
    debug = False

    # detector = cv2.ORB_create(nfeatures=500)
    # kps, des = grid_detect(detector, self.img0, **kwargs)
    # self.assertTrue(len(kps) > 0)
    # self.assertEqual(des.shape[0], len(kps))

    detector = cv2.FastFeatureDetector_create(threshold=50)
    kwargs = {'optflow_mode': True, 'debug': debug}
    kps = grid_detect(detector, self.img0, **kwargs)
    self.assertTrue(len(kps) > 0)

  def test_optflow_track(self):
    """ Test optflow_track() """
    debug = False

    # Detect
    feature = cv2.ORB_create(nfeatures=100)
    kps, des = grid_detect(feature, self.img0)
    self.assertTrue(len(kps) == len(des))

    # Track
    pts_i = np.array([kp.pt for kp in kps], dtype=np.float32)
    track_results = optflow_track(self.img0, self.img1, pts_i, debug=debug)
    (pts_i, pts_j, inliers) = track_results

    self.assertTrue(len(pts_i) == len(pts_j))
    self.assertTrue(len(pts_i) == len(inliers))


class TestFeatureTracker(unittest.TestCase):
  """ Test FeatureTracker """
  @classmethod
  def setUpClass(cls):
    super(TestFeatureTracker, cls).setUpClass()
    cls.dataset = EurocDataset(euroc_data_path)

  def setUp(self):
    # Setup test images
    self.dataset = TestFeatureTracker.dataset
    ts = self.dataset.cam0_data.timestamps[0]
    img0_path = self.dataset.cam0_data.image_paths[ts]
    img1_path = self.dataset.cam1_data.image_paths[ts]
    self.img0 = cv2.imread(img0_path, cv2.IMREAD_GRAYSCALE)
    self.img1 = cv2.imread(img1_path, cv2.IMREAD_GRAYSCALE)

    # Setup cameras
    # -- cam0
    res = self.dataset.cam0_data.config.resolution
    proj_params = self.dataset.cam0_data.config.intrinsics
    dist_params = self.dataset.cam0_data.config.distortion_coefficients
    proj_model = "pinhole"
    dist_model = "radtan4"
    params = np.block([*proj_params, *dist_params])
    cam0 = camera_params_setup(0, res, proj_model, dist_model, params)
    # -- cam1
    res = self.dataset.cam1_data.config.resolution
    proj_params = self.dataset.cam1_data.config.intrinsics
    dist_params = self.dataset.cam1_data.config.distortion_coefficients
    proj_model = "pinhole"
    dist_model = "radtan4"
    params = np.block([*proj_params, *dist_params])
    cam1 = camera_params_setup(1, res, proj_model, dist_model, params)

    # Setup camera extrinsics
    # -- cam0
    T_BC0 = self.dataset.cam0_data.config.T_BS
    cam0_exts = extrinsics_setup(T_BC0)
    # -- cam1
    T_BC1 = self.dataset.cam1_data.config.T_BS
    cam1_exts = extrinsics_setup(T_BC1)

    # Setup feature tracker
    self.feature_tracker = FeatureTracker()
    self.feature_tracker.add_camera(0, cam0, cam0_exts)
    self.feature_tracker.add_camera(1, cam1, cam1_exts)
    self.feature_tracker.add_overlap(0, 1)

  def test_detect(self):
    """ Test FeatureTracker._detect() """
    # Load and detect features from single image
    kps = self.feature_tracker._detect(self.img0)
    self.assertTrue(len(kps) > 0)

  def test_detect_overlaps(self):
    """ Test FeatureTracker._detect_overlaps() """
    debug = False
    # debug = True

    # Feed camera images to feature tracker
    mcam_imgs = {0: self.img0, 1: self.img1}
    self.feature_tracker._detect_overlaps(mcam_imgs)

    # Assert
    data_i = self.feature_tracker.cam_data[0]
    data_j = self.feature_tracker.cam_data[1]
    kps_i = data_i.keypoints
    kps_j = data_j.keypoints
    overlapping_ids = self.feature_tracker.feature_overlaps

    self.assertTrue(len(kps_i) == len(kps_j))
    self.assertTrue(len(kps_i) == len(overlapping_ids))

    # Visualize
    for cam_i, overlaps in self.feature_tracker.cam_overlaps.items():
      cam_j = overlaps[0]
      img_i = mcam_imgs[cam_i]
      img_j = mcam_imgs[cam_j]
      data_i = self.feature_tracker.cam_data[cam_i]
      data_j = self.feature_tracker.cam_data[cam_j]
      kps_i = data_i.keypoints
      kps_j = data_j.keypoints
      # viz = draw_matches(img_i, img_j, kps_i, kps_j)

      matches = []
      for i in range(len(kps_i)):
        matches.append(cv2.DMatch(i, i, 0))
      viz = cv2.drawMatches(img_i, kps_i, img_j, kps_j, matches, None)

      if debug:
        cv2.imshow('viz', viz)
        cv2.waitKey(0)

  def test_detect_nonoverlaps(self):
    """ Test FeatureTracker._detect_nonoverlaps() """
    # Feed camera images to feature tracker
    mcam_imgs = {0: self.img0, 1: self.img1}
    self.feature_tracker._detect_nonoverlaps(mcam_imgs)

    # Visualize
    for cam_i, overlaps in self.feature_tracker.cam_overlaps.items():
      cam_j = overlaps[0]
      img_i = mcam_imgs[cam_i]
      img_j = mcam_imgs[cam_j]
      data_i = self.feature_tracker.cam_data[cam_i]
      data_j = self.feature_tracker.cam_data[cam_j]
      kps_i = data_i.keypoints
      kps_j = data_j.keypoints

      viz_i = cv2.drawKeypoints(img_i, kps_i, None)
      viz_j = cv2.drawKeypoints(img_j, kps_j, None)
      viz = cv2.hconcat([viz_i, viz_j])

      debug = False
      # debug = True
      if debug:
        cv2.imshow('viz', viz)
        cv2.waitKey(0)

  def test_detect_new(self):
    """ Test FeatureTracker.detect_new() """
    mcam_imgs = {0: self.img0, 1: self.img1}
    self.feature_tracker._detect_new(mcam_imgs)
    ft_data = self.feature_tracker.cam_data
    viz = visualize_tracking(ft_data)

    debug = False
    # debug = True
    if debug:
      cv2.imshow('viz', viz)
      cv2.waitKey(0)

  def test_update(self):
    """ Test FeatureTracker.update() """
    for ts in self.dataset.cam0_data.timestamps[1000:1200]:
      # for ts in self.dataset.cam0_data.timestamps:
      # Load images
      img0_path = self.dataset.cam0_data.image_paths[ts]
      img1_path = self.dataset.cam1_data.image_paths[ts]
      img0 = cv2.imread(img0_path, cv2.IMREAD_GRAYSCALE)
      img1 = cv2.imread(img1_path, cv2.IMREAD_GRAYSCALE)

      # Feed camera images to feature tracker
      mcam_imgs = {0: img0, 1: img1}
      ft_data = self.feature_tracker.update(ts, mcam_imgs)

      # Visualize
      debug = False
      # debug = True
      if debug:
        sys.stdout.flush()
        viz = visualize_tracking(ft_data)
        cv2.imshow('viz', viz)
        if cv2.waitKey(1) == ord('q'):
          break
    cv2.destroyAllWindows()


class TestTracker(unittest.TestCase):
  """ Test Tracker """
  @classmethod
  def setUpClass(cls):
    super(TestTracker, cls).setUpClass()
    # Load dataset
    cls.dataset = EurocDataset(euroc_data_path)
    ts0 = cls.dataset.cam0_data.timestamps[0]
    cls.img0 = cls.dataset.get_camera_image(0, ts0)
    cls.img1 = cls.dataset.get_camera_image(1, ts0)

    # Imu params
    noise_acc = 0.08  # accelerometer measurement noise stddev.
    noise_gyr = 0.004  # gyroscope measurement noise stddev.
    noise_ba = 0.00004  # accelerometer bias random work noise stddev.
    noise_bg = 2.0e-6  # gyroscope bias random work noise stddev.
    cls.imu_params = ImuParams(noise_acc, noise_gyr, noise_ba, noise_bg)

    # Setup cameras
    # -- cam0
    res = cls.dataset.cam0_data.config.resolution
    proj_params = cls.dataset.cam0_data.config.intrinsics
    dist_params = cls.dataset.cam0_data.config.distortion_coefficients
    proj_model = "pinhole"
    dist_model = "radtan4"
    params = np.block([*proj_params, *dist_params])
    cls.cam0 = camera_params_setup(0, res, proj_model, dist_model, params)
    cls.cam0.fix = True
    # -- cam1
    res = cls.dataset.cam1_data.config.resolution
    proj_params = cls.dataset.cam1_data.config.intrinsics
    dist_params = cls.dataset.cam1_data.config.distortion_coefficients
    proj_model = "pinhole"
    dist_model = "radtan4"
    params = np.block([*proj_params, *dist_params])
    cls.cam1 = camera_params_setup(1, res, proj_model, dist_model, params)
    cls.cam1.fix = True

    # Setup camera extrinsics
    # -- cam0
    T_BC0 = cls.dataset.cam0_data.config.T_BS
    cls.cam0_exts = extrinsics_setup(T_BC0)
    cls.cam0_exts.fix = True
    # -- cam1
    T_BC1 = cls.dataset.cam1_data.config.T_BS
    cls.cam1_exts = extrinsics_setup(T_BC1)
    cls.cam1_exts.fix = True

  def setUp(self):
    # Setup test dataset
    self.dataset = TestTracker.dataset
    self.imu_params = TestTracker.imu_params
    self.cam0 = TestTracker.cam0
    self.cam1 = TestTracker.cam1
    self.cam0_exts = TestTracker.cam0_exts
    self.cam1_exts = TestTracker.cam1_exts

    # Setup tracker
    ts0 = self.dataset.ground_truth.timestamps[0]
    T_WB = self.dataset.ground_truth.T_WB[ts0]

    feature_tracker = FeatureTracker()
    self.tracker = Tracker(feature_tracker)
    self.tracker.add_imu(self.imu_params)
    self.tracker.add_camera(0, self.cam0, self.cam0_exts)
    self.tracker.add_camera(1, self.cam1, self.cam1_exts)
    self.tracker.add_overlap(0, 1)
    self.tracker.set_initial_pose(T_WB)

  def test_tracker_add_camera(self):
    """ Test Tracker.add_camera() """
    self.assertTrue(len(self.tracker.cam_params), 2)
    self.assertTrue(len(self.tracker.cam_geoms), 2)
    self.assertTrue(len(self.tracker.cam_exts), 2)

  def test_tracker_set_initial_pose(self):
    """ Test Tracker.set_initial_pose() """
    self.assertTrue(self.tracker.pose_init is not None)

  def test_tracker_inertial_callback(self):
    """ Test Tracker.inertial_callback() """
    ts = 0
    acc = np.array([0.0, 0.0, 10.0])
    gyr = np.array([0.0, 0.0, 0.0])
    self.tracker.inertial_callback(ts, acc, gyr)
    self.assertEqual(self.tracker.imu_buf.length(), 1)
    self.assertTrue(self.tracker.imu_started)

  def test_tracker_triangulate(self):
    """ Test Tracker._triangulate() """
    # Feature in world frame
    p_W = np.array([1.0, 0.01, 0.02])

    # Body pose in world frame
    C_WB = euler321(*deg2rad([-90.0, 0.0, -90.0]))
    r_WB = np.array([0.0, 0.0, 0.0])
    T_WB = tf(C_WB, r_WB)

    # Camera parameters and geometry
    cam_i = 0
    cam_j = 1
    cam_params_i = self.tracker.cam_params[cam_i]
    cam_params_j = self.tracker.cam_params[cam_j]
    cam_geom_i = self.tracker.cam_geoms[cam_i]
    cam_geom_j = self.tracker.cam_geoms[cam_j]

    # Camera extrinsics
    T_BCi = pose2tf(self.tracker.cam_exts[cam_i].param)
    T_BCj = pose2tf(self.tracker.cam_exts[cam_j].param)

    # Point relative to cam_i and cam_j
    p_Ci = tf_point(inv(T_WB @ T_BCi), p_W)
    p_Cj = tf_point(inv(T_WB @ T_BCj), p_W)

    # Image point z_i and z_j
    status_i, z_i = cam_geom_i.project(cam_params_i.param, p_Ci)
    status_j, z_j = cam_geom_j.project(cam_params_j.param, p_Cj)

    # Triangulate
    p_W_est = self.tracker._triangulate(cam_i, cam_j, z_i, z_j, T_WB)

    # Assert
    self.assertTrue(status_i)
    self.assertTrue(status_j)
    self.assertTrue(np.allclose(p_W_est, p_W))

  def test_tracker_add_pose(self):
    """ Test Tracker._add_pose() """
    # Timestamp
    ts = 0

    # Body pose in world frame
    C_WB = euler321(*deg2rad([-90.0, 0.0, -90.0]))
    r_WB = np.array([0.0, 0.0, 0.0])
    T_WB = tf(C_WB, r_WB)

    # Add pose
    pose = self.tracker._add_pose(ts, T_WB)
    self.assertTrue(pose is not None)

  def test_tracker_add_feature(self):
    """ Test Tracker._add_feature() """
    # Feature in world frame
    p_W = np.array([1.0, 0.01, 0.02])

    # Body pose in world frame
    C_WB = euler321(*deg2rad([-90.0, 0.0, -90.0]))
    r_WB = np.array([0.0, 0.0, 0.0])
    T_WB = tf(C_WB, r_WB)

    # Project world point to image plane
    cam_idx = 0
    cam_params = self.tracker.cam_params[cam_idx]
    cam_geom = self.tracker.cam_geoms[cam_idx]
    T_BC = pose2tf(self.tracker.cam_exts[cam_idx].param)
    p_C = tf_point(inv(T_WB @ T_BC), p_W)
    status, z = cam_geom.project(cam_params.param, p_C)

    # Add feature
    fid = 0
    ts = 0
    kp = cv2.KeyPoint(z[0], z[1], 0)
    self.tracker._add_feature(fid, ts, cam_idx, kp)

    # Assert
    self.assertTrue(status)
    self.assertTrue(fid in self.tracker.features)
    self.assertEqual(len(self.tracker.features), 1)

  def test_tracker_update_feature(self):
    """ Test Tracker._update_feature() """
    # Feature in world frame
    p_W = np.array([1.0, 0.01, 0.02])

    # Body pose in world frame
    C_WB = euler321(*deg2rad([-90.0, 0.0, -90.0]))
    r_WB = np.array([0.0, 0.0, 0.0])
    T_WB = tf(C_WB, r_WB)

    # Camera parameters and geometry
    cam_i = 0
    cam_j = 1
    cam_params_i = self.tracker.cam_params[cam_i]
    cam_params_j = self.tracker.cam_params[cam_j]
    cam_geom_i = self.tracker.cam_geoms[cam_i]
    cam_geom_j = self.tracker.cam_geoms[cam_j]

    # Project p_W to image point z_i and z_j
    T_BCi = pose2tf(self.tracker.cam_exts[cam_i].param)
    T_BCj = pose2tf(self.tracker.cam_exts[cam_j].param)
    p_Ci = tf_point(inv(T_WB @ T_BCi), p_W)
    p_Cj = tf_point(inv(T_WB @ T_BCj), p_W)
    status_i, z_i = cam_geom_i.project(cam_params_i.param, p_Ci)
    status_j, z_j = cam_geom_j.project(cam_params_j.param, p_Cj)

    # Add feature
    fid = 0
    ts = 0
    kp_i = cv2.KeyPoint(z_i[0], z_i[1], 0)
    kp_j = cv2.KeyPoint(z_j[0], z_j[1], 0)
    self.tracker._add_feature(fid, ts, cam_i, kp_i)
    self.tracker._update_feature(fid, ts, cam_j, kp_j, T_WB)

    # Assert
    feature = self.tracker.features[fid]
    p_W_est = feature.param
    self.assertTrue(status_i)
    self.assertTrue(status_j)
    self.assertTrue(fid in self.tracker.features)
    self.assertEqual(len(self.tracker.features), 1)
    self.assertTrue(feature.data.initialized())
    self.assertTrue(np.allclose(p_W_est, p_W))

  def test_tracker_process_features(self):
    """ Test Tracker._process_features() """

    for ts in self.dataset.cam0_data.timestamps:
      # Get ground truth pose
      T_WB = self.dataset.get_ground_truth_pose(ts)
      if T_WB is None:
        continue

      # Feed camera images to feature tracker
      img0 = self.dataset.get_camera_image(0, ts)
      img1 = self.dataset.get_camera_image(1, ts)
      ft_data = self.tracker.feature_tracker.update(ts, {0: img0, 1: img1})

      # Process features
      pose = self.tracker._add_pose(ts, T_WB)
      self.tracker._process_features(ts, ft_data, pose)
      self.assertTrue(self.tracker.nb_features() > 0)
      break

  def test_tracker_add_keyframe(self):
    """ Test Tracker._add_keyframe() """
    for ts in self.dataset.cam0_data.timestamps:
      # Get ground truth pose
      T_WB = self.dataset.get_ground_truth_pose(ts)
      if T_WB is None:
        continue

      # Feed camera images to feature tracker
      img0 = self.dataset.get_camera_image(0, ts)
      img1 = self.dataset.get_camera_image(1, ts)
      mcam_imgs = {0: img0, 1: img1}
      ft_data = self.tracker.feature_tracker.update(ts, mcam_imgs)

      # Process features
      pose = self.tracker._add_pose(ts, T_WB)
      self.tracker._process_features(ts, ft_data, pose)
      self.tracker._add_keyframe(ts, mcam_imgs, ft_data, pose)
      self.assertTrue(self.tracker.nb_features() > 0)
      self.assertEqual(self.tracker.nb_keyframes(), 1)
      break

  # @unittest.skip("")
  def test_tracker_vision_callback(self):
    """ Test Tracker.vision_callback() """
    # Disable imu in Tracker
    self.tracker.imu_params = None

    # Create csv files
    pose_est_csv = open("/tmp/poses_est.csv", "w")
    pose_gnd_csv = open("/tmp/poses_gnd.csv", "w")
    pose_est_csv.write("ts,rx,ry,rz,qw,qx,qy,qz\n")
    pose_gnd_csv.write("ts,rx,ry,rz,qw,qx,qy,qz\n")
    poses_est = []
    poses_gnd = []

    # Loop through timestamps
    for k, ts in enumerate(self.dataset.cam0_data.timestamps[0:300]):
      # Get ground truth pose
      T_WB = self.dataset.get_ground_truth_pose(ts)
      if T_WB is None:
        continue

      # Vision callback
      img0 = self.dataset.get_camera_image(0, ts)
      img1 = self.dataset.get_camera_image(1, ts)
      self.tracker.vision_callback(ts, {0: img0, 1: img1})

      # print(f"{ts}, {self.tracker.nb_features()}")
      # self.assertTrue(self.tracker.nb_features() > 0)
      # self.assertEqual(self.tracker.nb_keyframes(), 1)

      last_kf = self.tracker.keyframes[-1]
      poses_est.append(tf2pose(pose2tf(last_kf.pose.param)))
      poses_gnd.append(tf2pose(T_WB))
      print(f"frame_idx: {k}")
      pose_est_csv.write("%ld,%f,%f,%f,%f,%f,%f,%f\n" % (ts, *poses_est[-1]))
      pose_gnd_csv.write("%ld,%f,%f,%f,%f,%f,%f,%f\n" % (ts, *poses_gnd[-1]))

    # Close csv files
    pose_est_csv.close()
    pose_gnd_csv.close()

    # Plot
    poses_gnd = pandas.read_csv("/tmp/poses_gnd.csv")
    poses_est = pandas.read_csv("/tmp/poses_est.csv")
    title = "Displacement"
    data = {"Ground Truth": poses_gnd, "Estimate": poses_est}
    plot_xyz(title, data, 'ts', 'rx', 'ry', 'rz', 'Displacement [m]')
    plt.show()


# CALIBRATION #################################################################


class TestCalibration(unittest.TestCase):
  """ Test calibration functions """
  def test_aprilgrid(self):
    """ Test aprilgrid """
    # grid = AprilGrid()
    # self.assertTrue(grid is not None)

    grid = AprilGrid.load(
        "/tmp/aprilgrid_test/mono/cam0/1403709383937837056.csv")
    self.assertTrue(grid is not None)

    dataset = EurocDataset(euroc_data_path)
    res = dataset.cam0_data.config.resolution
    proj_params = dataset.cam0_data.config.intrinsics
    dist_params = dataset.cam0_data.config.distortion_coefficients
    proj_model = "pinhole"
    dist_model = "radtan4"
    params = np.block([*proj_params, *dist_params])
    cam0 = camera_params_setup(0, res, proj_model, dist_model, params)

    grid.solvepnp(cam0)

    # debug = True
    debug = False
    if debug:
      _, ax = plt.subplots()
      for _, _, kp, _ in grid.get_measurements():
        ax.plot(kp[0], kp[1], 'r.')
      ax.xaxis.tick_top()
      ax.xaxis.set_label_position('top')
      ax.set_xlim([0, 752])
      ax.set_ylim([0, 480])
      ax.set_ylim(ax.get_ylim()[::-1])
      plt.show()

  def test_calib_generate_poses(self):
    """ Test calib_generate_poses() """
    # Calibration target
    calib_target = AprilGrid()
    poses = calib_generate_poses(calib_target)
    self.assertTrue(len(poses) > 0)

    # Calibration target pose in world frame
    C_WF = euler321(-pi / 2.0, 0.0, deg2rad(80.0))
    r_WF = np.array([0.0, 0.0, 0.0])
    T_WF = tf(C_WF, r_WF)

    # debug = True
    debug = False
    if debug:
      plt.figure()
      ax = plt.axes(projection='3d')

      calib_target.plot(ax, T_WF)
      for T_FC in poses:
        plot_tf(ax, T_WF @ T_FC, size=0.05)

      plot_set_axes_equal(ax)
      ax.set_xlabel("x [m]")
      ax.set_ylabel("y [m]")
      ax.set_zlabel("z [m]")
      plt.show()

  def test_calib_generate_random_poses(self):
    """ Test calib_generate_random_poses() """
    # Calibration target
    calib_target = AprilGrid()
    poses = calib_generate_random_poses(calib_target)
    self.assertTrue(len(poses) > 0)

    # Calibration target pose in world frame
    C_WF = euler321(-pi / 2.0, 0.0, deg2rad(80.0))
    r_WF = np.array([0.0, 0.0, 0.0])
    T_WF = tf(C_WF, r_WF)

    # debug = True
    debug = False
    if debug:
      plt.figure()
      ax = plt.axes(projection='3d')

      calib_target.plot(ax, T_WF)
      for T_FC in poses:
        plot_tf(ax, T_WF @ T_FC, size=0.05)

      plot_set_axes_equal(ax)
      ax.set_xlabel("x [m]")
      ax.set_ylabel("y [m]")
      ax.set_zlabel("z [m]")
      plt.show()

  # @unittest.skip("")
  def test_calibrator(self):
    """ Test Calibrator """
    # import apriltag_pybind as apriltag
    #
    # cam0_imgs = glob.glob("/data/euroc/cam_april/mav0/cam0/data/*.png")
    # cam0_imgs = sorted(cam0_imgs)
    #
    # # Loop through images
    # imshow_timeout = 1
    # for img_path in cam0_imgs:
    #   img = cv2.imread(img_path, cv2.IMREAD_GRAYSCALE)
    #   tag_data = apriltag.detect(img)
    #   if not tag_data:
    #     continue
    #
    #   # Visualize detection
    #   viz = cv2.cvtColor(img, cv2.COLOR_GRAY2BGR)
    #   for (tag_id, corner_idx, kp_x, kp_y) in tag_data:
    #     pt = (int(kp_x), int(kp_y))
    #     radius = 5
    #     color = (0, 0, 255)
    #     thickness = 2
    #     cv2.circle(viz, pt, radius, color, thickness)
    #   cv2.imshow("viz", viz)
    #   key = cv2.waitKey(imshow_timeout)
    #   if key == ord('q'):
    #     break
    #   elif key == ord('p'):
    #     imshow_timeout = 0 if imshow_timeout else 1
    #   elif key == ord('s'):
    #     imshow_timeout = 0

    # Setup
    grid_csvs = glob.glob("/data/euroc/cam_april/mav0/grid0/cam0/*.csv")
    grids = [AprilGrid.load(csv_path) for csv_path in grid_csvs]
    self.assertTrue(len(grid_csvs) > 0)
    self.assertTrue(len(grids) > 0)

    # Calibrator
    calib = Calibrator()
    # -- Add cam0
    cam_idx = 0
    cam_res = [752, 480]
    proj_model = "pinhole"
    dist_model = "radtan4"
    calib.add_camera(cam_idx, cam_res, proj_model, dist_model)
    # -- Add camera views
    for grid in grids:
      if grid is not None:
        calib.add_camera_view(grid.ts, cam_idx, grid)
        if calib.get_num_views() == 10:
          break
    # -- Solve
    calib.solve()


# SIMULATION  #################################################################


class TestSimulation(unittest.TestCase):
  """ Test simulation functions """
  def test_create_3d_features(self):
    """ Test create 3D features """
    debug = False
    x_bounds = np.array([-10.0, 10.0])
    y_bounds = np.array([-10.0, 10.0])
    z_bounds = np.array([-10.0, 10.0])
    nb_features = 1000
    features = create_3d_features(x_bounds, y_bounds, z_bounds, nb_features)
    self.assertTrue(features.shape == (nb_features, 3))

    if debug:
      fig = plt.figure()
      ax = fig.gca(projection='3d')
      ax.scatter(features[:, 0], features[:, 1], features[:, 2])
      ax.set_xlabel("x [m]")
      ax.set_ylabel("y [m]")
      ax.set_zlabel("z [m]")
      plt.show()

  def test_create_3d_features_perimeter(self):
    """ Test create_3d_features_perimeter() """
    debug = False
    origin = np.array([0.0, 0.0, 0.0])
    dim = np.array([10.0, 10.0, 5.0])
    nb_features = 1000
    features = create_3d_features_perimeter(origin, dim, nb_features)
    self.assertTrue(features.shape == (nb_features, 3))

    if debug:
      fig = plt.figure()
      ax = fig.gca(projection='3d')
      ax.scatter(features[:, 0], features[:, 1], features[:, 2])
      ax.set_xlabel("x [m]")
      ax.set_ylabel("y [m]")
      ax.set_zlabel("z [m]")
      plt.show()

  def test_sim_camera_frame(self):
    """ Test SimCameraFrame() """
    # Camera properties
    cam_idx = 0
    img_w = 640
    img_h = 480
    res = [img_w, img_h]
    fov = 120.0
    fx = focal_length(img_w, fov)
    fy = focal_length(img_w, fov)
    cx = img_w / 2.0
    cy = img_h / 2.0

    # Camera parameters
    proj_model = "pinhole"
    dist_model = "radtan4"
    proj_params = [fx, fy, cx, cy]
    dist_params = [0.0, 0.0, 0.0, 0.0]
    params = np.block([*proj_params, *dist_params])
    camera = camera_params_setup(cam_idx, res, proj_model, dist_model, params)

    # Features
    features = []
    for i in np.linspace(-2.0, 2.0, 5):
      for j in np.linspace(-2.0, 2.0, 5):
        x = 1.0
        y = j
        z = i
        features.append(np.array([x, y, z]))
    features = np.array(features)

    # Camera pose
    C_WC0 = euler321(*deg2rad([-90.0, 0.0, -90.0]))
    r_WC0 = np.array([0.0, 0.0, 0.0])
    T_WC0 = tf(C_WC0, r_WC0)

    # Camera frame
    ts = 0
    cam_frame = SimCameraFrame(ts, cam_idx, camera, T_WC0, features)
    self.assertEqual(len(cam_frame.measurements), 9)

    # Visualize
    # debug = True
    debug = False
    if debug:
      kps = [kp for kp in cam_frame.measurements]
      img0 = np.zeros((img_h, img_w), dtype=np.uint8)
      viz = draw_keypoints(img0, kps)
      cv2.imshow('viz', viz)
      cv2.waitKey(0)

  def test_sim_data(self):
    """ Test SimData() """
    debug_cam = False
    debug_imu = False

    # Sim data
    circle_r = 5.0
    circle_v = 1.0
    pickle_path = '/tmp/sim_data.pickle'
    sim_data = SimData.create_or_load(circle_r, circle_v, pickle_path)
    cam0_data = sim_data.mcam_data[0]
    cam1_data = sim_data.mcam_data[1]

    self.assertTrue(sim_data is not None)
    self.assertTrue(sim_data.features.shape[0] > 0)
    self.assertTrue(sim_data.features.shape[1] == 3)
    self.assertTrue(cam0_data.cam_idx == 0)
    self.assertTrue(len(cam0_data.poses) == len(cam0_data.frames))
    self.assertTrue(cam1_data.cam_idx == 1)
    self.assertTrue(len(cam1_data.poses) == len(cam1_data.frames))

    if debug_cam:
      cam0_data = sim_data.mcam_data[0]
      pos = np.array([tf_trans(v) for k, v in cam0_data.poses.items()])

      plt.figure()
      plt.plot(pos[:, 0], pos[:, 1], 'r-')
      plt.xlabel("Displacement [m]")
      plt.ylabel("Displacement [m]")
      plt.title("Camera Position")
      plt.subplots_adjust(hspace=0.9)
      plt.show()

    if debug_imu:
      imu0_data = sim_data.imu0_data

      pos = np.array([tf_trans(v) for k, v in imu0_data.poses.items()])
      vel = np.array([v for k, v in imu0_data.vel.items()])
      acc = np.array([v for k, v in imu0_data.acc.items()])
      gyr = np.array([v for k, v in imu0_data.gyr.items()])

      plt.figure()
      plt.subplot(411)
      plt.plot(pos[:, 0], pos[:, 1], 'r-')
      plt.xlabel("Time [s]")
      plt.ylabel("Displacement [m]")
      plt.title("IMU Position")

      plt.subplot(412)
      plt.plot(imu0_data.timestamps, vel[:, 0], 'r-')
      plt.plot(imu0_data.timestamps, vel[:, 1], 'g-')
      plt.plot(imu0_data.timestamps, vel[:, 2], 'b-')
      plt.xlabel("Time [s]")
      plt.ylabel("Velocity [ms^-1]")
      plt.title("IMU Velocity")

      plt.subplot(413)
      plt.plot(imu0_data.timestamps, acc[:, 0], 'r-')
      plt.plot(imu0_data.timestamps, acc[:, 1], 'g-')
      plt.plot(imu0_data.timestamps, acc[:, 2], 'b-')
      plt.xlabel("Time [s]")
      plt.ylabel("Acceleration [ms^-2]")
      plt.title("Accelerometer Measurements")

      plt.subplot(414)
      plt.plot(imu0_data.timestamps, gyr[:, 0], 'r-')
      plt.plot(imu0_data.timestamps, gyr[:, 1], 'g-')
      plt.plot(imu0_data.timestamps, gyr[:, 2], 'b-')
      plt.xlabel("Time [s]")
      plt.ylabel("Angular Velocity [rad s^-1]")
      plt.title("Gyroscope Measurements")

      plt.subplots_adjust(hspace=0.9)
      plt.show()

  def test_sim_feature_tracker(self):
    """ Test SimFeatureTracker """
    # Sim data
    circle_r = 5.0
    circle_v = 1.0
    pickle_path = '/tmp/sim_data.pickle'
    sim_data = SimData.create_or_load(circle_r, circle_v, pickle_path)
    cam0_params = sim_data.get_camera_params(0)
    cam1_params = sim_data.get_camera_params(1)
    cam0_exts = sim_data.get_camera_extrinsics(0)
    cam1_exts = sim_data.get_camera_extrinsics(1)

    # Sim feature tracker
    feature_tracker = SimFeatureTracker()
    feature_tracker.add_camera(0, cam0_params, cam0_exts)
    feature_tracker.add_camera(1, cam1_params, cam1_exts)
    feature_tracker.add_overlap(0, 1)

    # Loop through timeline events
    mcam_buf = MultiCameraBuffer(2)
    for ts in sim_data.timeline.get_timestamps():
      for event in sim_data.timeline.get_events(ts):
        if isinstance(event, CameraEvent):

          mcam_buf.add(event.ts, event.cam_idx, event.image)
          if mcam_buf.ready():
            mcam_data = mcam_buf.get_data()
            ft_data = feature_tracker.update(ts, mcam_data)
            mcam_buf.reset()

            self.assertTrue(ft_data is not None)
            self.assertTrue(ft_data[0].keypoints)
            self.assertTrue(ft_data[1].keypoints)
            self.assertTrue(ft_data[0].feature_ids)
            self.assertTrue(ft_data[1].feature_ids)


# VISUALIZER ###################################################################


async def fake_loop(ws, _):
  """ Simulates a simulation or dev loop """
  # Setup plots
  print("Connected to client!")
  multi_plot = MultiPlot(has_gnd=True)
  await ws.send(multi_plot.get_plots())

  # Loop
  index = 0

  while True:
    index += 1

    t = index
    x = np.random.random()
    y = np.random.random()
    z = np.random.random()
    gnd = np.random.random(3)
    est = np.random.random(3)
    multi_plot.add_pos_xy_data(est=est, gnd=gnd)
    multi_plot.add_pos_z_data(t, est=z, gnd=x)
    multi_plot.add_roll_data(t, est=x, gnd=y)
    multi_plot.add_pitch_data(t, est=x, gnd=y)
    multi_plot.add_yaw_data(t, est=x, gnd=y)
    multi_plot.add_pos_error_data(t, y)
    multi_plot.add_att_error_data(t, x)
    multi_plot.add_reproj_error_data(t, x, y)
    await multi_plot.emit_data(ws)

  # Important
  await ws.close()
  DevServer.stop()


class TestViz(unittest.TestCase):
  """ Test Viz """
  def test_multiplot(self):
    """ Test MultiPlot() """
    t = 0
    x = np.random.random()
    y = np.random.random()
    z = np.random.random()
    gnd = np.random.random(3)
    est = np.random.random(3)
    multi_plot = MultiPlot(has_gnd=True)
    multi_plot.add_pos_xy_data(est=est, gnd=gnd)
    multi_plot.add_pos_z_data(t, est=z, gnd=x)
    multi_plot.add_roll_data(t, est=x, gnd=y)
    multi_plot.add_pitch_data(t, est=x, gnd=y)
    multi_plot.add_yaw_data(t, est=x, gnd=y)
    multi_plot.add_pos_error_data(t, y)
    multi_plot.add_att_error_data(t, x)
    multi_plot.add_reproj_error_data(t, x, y)

    # import pprint
    # pprint.pprint(multi_plot.get_plots())

    self.assertTrue(multi_plot is not None)

  @unittest.skip("")
  def test_server(self):
    """ Test DevServer() """
    viz_server = DevServer(fake_loop)
    viz_server.run()
    self.assertTrue(viz_server is not None)


if __name__ == '__main__':
  unittest.main(failfast=True)